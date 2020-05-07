/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
// import { v3 as uuidv3 } from 'uuid'
import createWidget from '../widget/index'
import { getLocale } from '../../../../common/locale'

import {
  WidgetWrapper,
  Header,
  Content,
  WelcomeText,
  ActionsWrapper,
  CallButton,
  TogetherIcon,
  StyledTitle,
  StyledTitleText
} from './style'
import { StyledTitleTab } from '../widgetTitleTab'
import BraveTogetherIcon from './assets/brave-together-icon'

interface Props {
  showContent: boolean
  onShowContent: () => void
}

interface State {
  room: string
}

class Together extends React.PureComponent<Props, State> {

  constructor (props: Props) {
    super(props)
    this.state = {
      room: ''
    }
    console.log(this.state)
  }

  getButtonText = () => {
    return getLocale('togetherWidgetStartButton')
  }

  renderTitle () {
    return (
      <Header>
        <StyledTitle>
          <TogetherIcon>
            <BraveTogetherIcon />
          </TogetherIcon>
          <StyledTitleText>
          {getLocale('togetherWidgetTitle')}
          </StyledTitleText>
        </StyledTitle>
      </Header>
    )
  }

  renderTitleTab () {
    const { onShowContent } = this.props

    return (
      <StyledTitleTab onClick={onShowContent}>
        {this.renderTitle()}
      </StyledTitleTab>
    )
  }

  roomname() {
    return 'bxxxxxxx'.replace(/[xy]/g, (c) => {
      let r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8)
      return v.toString(16)
    })
  }

  shouldCreateCall = (event: any) => {
    event.preventDefault()

    let { room } = this.state

    if (!room) {
      room = this.roomname()
    }

    window.open(`https://together.brave.com/${room}`, '_self')
  }

  render () {
    const {
      showContent
    } = this.props

    if (!showContent) {
      return this.renderTitleTab()
    }

    return (
      <WidgetWrapper>
          {this.renderTitle()}
          <Content>
            <WelcomeText>
              {getLocale('togetherWidgetWelcomeTitle')}
            </WelcomeText>
            <ActionsWrapper>
              <CallButton onClick={this.shouldCreateCall}>
                {getLocale('togetherWidgetStartButton')}
              </CallButton>
            </ActionsWrapper>
          </Content>
      </WidgetWrapper>
    )
  }
}

export const TogetherWidget = createWidget(Together)
