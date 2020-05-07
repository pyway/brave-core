/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledTitleTab = styled<{}, 'div'>('div')`
  color: #fff;
  cursor: pointer;
  padding: 4px 20px 15px 20px;
  margin-bottom: -8px;
  backdrop-filter: blur(23px);
  background: linear-gradient(45deg, rgba(33, 37, 41, 0.28) 0%, rgba(33, 37, 41, 0.22) 100%);
  border-radius: 8px 8px 0 0;
  box-shadow: rgba(0, 0, 0, 0.15) 1px 1px 20px 2px;

  &:first-child {
    background: linear-gradient(45deg, rgba(33, 37, 41, 0.22) 0%, rgba(33, 37, 41, 0.2) 100%);
  }
`
