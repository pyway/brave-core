/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/internal/credentials/credentials_promotion.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_promotion.h"
#include "bat/ledger/internal/request/request_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

std::string ParseClaimCredsResponse(const std::string& response) {
  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return "";
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return "";
  }

  auto* id = dictionary->FindStringKey("claimId");
  if (!id) {
    return "";
  }

  return *id;
}

}  // namespace

namespace braveledger_credentials {

CredentialsPromotion::CredentialsPromotion(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    common_(std::make_unique<CredentialsCommon>(ledger))  {
  DCHECK(ledger_ && common_);
}

CredentialsPromotion::~CredentialsPromotion() = default;

void CredentialsPromotion::Start(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto get_callback = std::bind(&CredentialsPromotion::OnStart,
          this,
          _1,
          trigger,
          callback);

  ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
}

void CredentialsPromotion::OnStart(
    ledger::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger::CredsBatchStatus status = ledger::CredsBatchStatus::NONE;
  if (creds) {
    status = creds->status;
  }

  switch (status) {
    case ledger::CredsBatchStatus::NONE:
    case ledger::CredsBatchStatus::BLINDED: {
      Blind(trigger, callback);
      break;
    }
    case ledger::CredsBatchStatus::CLAIMED: {
      auto get_callback = std::bind(&CredentialsPromotion::FetchSignedCreds,
          this,
          _1,
          trigger,
          callback);
      ledger_->GetPromotion(trigger.id, get_callback);
      break;
    }
    case ledger::CredsBatchStatus::SIGNED: {
      auto get_callback = std::bind(&CredentialsPromotion::Unblind,
          this,
          _1,
          trigger,
          callback);
      ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
      break;
    }
    case ledger::CredsBatchStatus::FINISHED: {
      callback(ledger::Result::LEDGER_OK);
      break;
    }
    case ledger::CredsBatchStatus::CORRUPTED: {
      callback(ledger::Result::LEDGER_ERROR);
      break;
    }
  }
}

void CredentialsPromotion::Blind(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto blinded_callback = std::bind(&CredentialsPromotion::Claim,
      this,
      _1,
      _2,
      trigger,
      callback);
  common_->GetBlindedCreds(trigger, blinded_callback);
}

void CredentialsPromotion::Claim(
    const ledger::Result result,
    const std::string& blinded_creds_json,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(blinded_creds_json);

  if (!blinded_creds || blinded_creds->empty()) {
    BLOG(0, "Blinded creds are corrupted");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string payment_id = ledger_->GetPaymentId();
  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", payment_id);
  body.SetKey("blindedCreds", base::Value(std::move(*blinded_creds)));

  std::string json;
  base::JSONWriter::Write(body, &json);

  ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();

  const auto headers = braveledger_request_util::BuildSignHeaders(
      "post /v1/promotions/" + trigger.id,
      json,
      payment_id,
      wallet_info.key_info_seed);

  const std::string url = braveledger_request_util::ClaimCredsUrl(trigger.id);
  auto url_callback = std::bind(&CredentialsPromotion::OnClaim,
      this,
      _1,
      _2,
      _3,
      trigger,
      callback);

  ledger_->LoadURL(
      url,
      headers,
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void CredentialsPromotion::OnClaim(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::ToString(__func__, response_status_code, response, headers));

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const auto claim_id = ParseClaimCredsResponse(response);

  if (claim_id.empty()) {
    BLOG(0, "Claim id is missing");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback = std::bind(&CredentialsPromotion::ClaimedSaved,
      this,
      _1,
      trigger,
      callback);

  ledger_->SavePromotionClaimId(trigger.id, claim_id, save_callback);
}

void CredentialsPromotion::ClaimedSaved(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Claim id was not saved");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback = std::bind(&CredentialsPromotion::ClaimStatusSaved,
      this,
      _1,
      trigger,
      callback);

  ledger_->UpdateCredsBatchStatus(
      trigger.id,
      trigger.type,
      ledger::CredsBatchStatus::CLAIMED,
      save_callback);
}

void CredentialsPromotion::ClaimStatusSaved(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Claim status not saved");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::FetchSignedCreds,
      this,
      _1,
      trigger,
      callback);
  ledger_->GetPromotion(trigger.id, get_callback);
}

void CredentialsPromotion::FetchSignedCreds(
    ledger::PromotionPtr promotion,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!promotion) {
    BLOG(0, "Corrupted data");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string url = braveledger_request_util::FetchSignedCredsUrl(
      trigger.id,
      promotion->claim_id);
  auto url_callback = std::bind(&CredentialsPromotion::OnFetchSignedCreds,
      this,
      _1,
      _2,
      _3,
      trigger,
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void CredentialsPromotion::OnFetchSignedCreds(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::ToString(__func__, response_status_code, response, headers));

  if (response_status_code == net::HTTP_ACCEPTED) {
    callback(ledger::Result::RETRY);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::SignedCredsSaved,
      this,
      _1,
      trigger,
      callback);
  common_->GetSignedCredsFromResponse(trigger, response, get_callback);
}

void CredentialsPromotion::SignedCredsSaved(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Signed creds were not saved");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::Unblind,
      this,
      _1,
      trigger,
      callback);
  ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
}

void CredentialsPromotion::Unblind(
    ledger::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(0, "Corrupted data");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsPromotion::VerifyPublicKey,
      this,
      _1,
      trigger,
      *creds,
      callback);
  ledger_->GetPromotion(trigger.id, get_callback);
}

void CredentialsPromotion::VerifyPublicKey(
    ledger::PromotionPtr promotion,
    const CredentialsTrigger& trigger,
    const ledger::CredsBatch& creds,
    ledger::ResultCallback callback) {
  if (!promotion) {
    BLOG(0, "Corrupted data");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto promotion_keys = ParseStringToBaseList(promotion->public_keys);

  if (!promotion_keys || promotion_keys->empty()) {
    BLOG(0, "Public key is missing");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  bool valid = false;
  for (auto& item : *promotion_keys) {
    if (item.GetString() == creds.public_key) {
      valid = true;
    }
  }

  if (!valid) {
    BLOG(0, "Public key is not valid");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> unblinded_encoded_creds;
  std::string error;
  bool result;
  if (ledger::is_testing) {
    result = UnBlindCredsMock(creds, &unblinded_encoded_creds);
  } else {
    result = UnBlindCreds(creds, &unblinded_encoded_creds, &error);
  }

  if (!result) {
    BLOG(0, "UnBlindTokens: " << error);
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const double cred_value =
      promotion->approximate_value / promotion->suggestions;

  auto save_callback = std::bind(&CredentialsPromotion::Completed,
      this,
      _1,
      trigger,
      callback);

  uint64_t expires_at = 0ul;
  if (promotion->type != ledger::PromotionType::ADS) {
    expires_at = promotion->expires_at;
  }

  common_->SaveUnblindedCreds(
      expires_at,
      cred_value,
      creds,
      unblinded_encoded_creds,
      trigger,
      save_callback);
}

void CredentialsPromotion::Completed(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Unblinded token save failed");
    callback(result);
    return;
  }

  ledger_->PromotionCredentialCompleted(trigger.id, callback);
  ledger_->UnblindedTokensReady();
}

void CredentialsPromotion::RedeemTokens(
    const CredentialsRedeem& redeem,
    ledger::ResultCallback callback) {
  if (redeem.token_list.empty()) {
    BLOG(0, "Token list empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> token_id_list;
  for (const auto & item : redeem.token_list) {
    token_id_list.push_back(base::NumberToString(item.id));
  }

  auto url_callback = std::bind(&CredentialsPromotion::OnRedeemTokens,
      this,
      _1,
      _2,
      _3,
      token_id_list,
      redeem,
      callback);

  std::string payload;
  std::string url;
  std::vector<std::string> headers;
  if (redeem.type == ledger::RewardsType::TRANSFER) {
    payload = GenerateTransferTokensPayload(redeem, ledger_->GetPaymentId());
    url = braveledger_request_util::GetTransferTokens();
    ledger::WalletInfoProperties wallet_info = ledger_->GetWalletInfo();
    headers = braveledger_request_util::BuildSignHeaders(
        "post /v1/suggestions/claim",
        payload,
        ledger_->GetPaymentId(),
        wallet_info.key_info_seed);
  } else {
    if (redeem.publisher_key.empty()) {
      BLOG(0, "Publisher key is empty");
      callback(ledger::Result::LEDGER_ERROR);
      return;
    }

    payload = GenerateRedeemTokensPayload(redeem);
    url = braveledger_request_util::GetReedemTokensUrl();
  }

  ledger_->LoadURL(
      url,
      headers,
      payload,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void CredentialsPromotion::OnRedeemTokens(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::vector<std::string>& token_id_list,
    const CredentialsRedeem& redeem,
    ledger::ResultCallback callback) {
  BLOG(6, ledger::ToString(__func__, response_status_code, response, headers));

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::string id;
  if (!redeem.contribution_id.empty()) {
    id = redeem.contribution_id;
  }

  ledger_->MarkUblindedTokensAsSpent(token_id_list, redeem.type, id, callback);
}

}  // namespace braveledger_credentials
