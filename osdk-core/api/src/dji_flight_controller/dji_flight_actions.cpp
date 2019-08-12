/** @file dji_flight_actions.cpp
 *  @version 3.9
 *  @date April 2019
 *
 *  @brief
 *  FlightActions API for DJI OSDK library
 *
 *  @Copyright (c) 2016-2019 DJI
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include "dji_flight_actions.hpp"
#include "dji_ack.hpp"
using namespace DJI;
using namespace DJI::OSDK;

FlightActions::FlightActions(ControlLink *controlLink)
    : controlLink(controlLink) {}

FlightActions::~FlightActions() {}
/*! TODO move this part's code to independent class (down)*/
void FlightActions::commonAckDecoder(Vehicle *vehicle, RecvContainer recvFrame,
                                     UCBRetCodeHandler *ucb) {
  if (ucb && ucb->UserCallBack) {
    CommonAck ack = {0};
    ErrorCode::ErrCodeType ret = 0;
    if (recvFrame.recvInfo.len - OpenProtocol::PackageMin >=
        sizeof(CommonAck)) {
      ack = *(CommonAck *)(recvFrame.recvData.raw_ack_array);
      ret = ErrorCode::UnifiedErrCode::kNoError;
    } else {
      DERROR("ACK is exception, data len %d (expect >= %d)\n",
             recvFrame.recvInfo.len - OpenProtocol::PackageMin,
             sizeof(CommonAck));
      ret = ErrorCode::UnifiedErrCode::kErrorInvalidRespond;
    }
    ucb->UserCallBack(
        (ret != ErrorCode::UnifiedErrCode::kNoError) ? ret : ack.ret_code,
        ucb->userData);
  }
}

FlightActions::UCBRetCodeHandler *FlightActions::allocUCBHandler(
    void *callback, UserData userData) {
  static int ucbHandlerIndex = 0;
  ucbHandlerIndex++;
  if (ucbHandlerIndex >= maxSize) {
    ucbHandlerIndex = 0;
  }
  ucbHandler[ucbHandlerIndex].UserCallBack =
      (void (*)(ErrorCode::ErrCodeType errCode, UserData userData))callback;
  ucbHandler[ucbHandlerIndex].userData = userData;
  return &(ucbHandler[ucbHandlerIndex]);
}

template <typename ReqT>
ErrorCode::ErrCodeType FlightActions::actionSync(ReqT req, int timeout) {
  if (controlLink) {
    ACK::ErrorCode *rsp = (ACK::ErrorCode *)controlLink->sendSync(
        OpenProtocolCMD::CMDSet::Control::task, (void *)&req, sizeof(req),
        timeout);
    if (rsp->info.buf &&
        (rsp->info.len - OpenProtocol::PackageMin >= sizeof(CommonAck))) {
      return ((CommonAck *)rsp->info.buf)->ret_code;
    } else {
      return ErrorCode::UnifiedErrCode::kErrorInvalidRespond;
    }
  } else
    return ErrorCode::UnifiedErrCode::kErrorSystemError;
}
/*! TODO move this part's code to independent class (up)*/

template <typename ReqT>
void FlightActions::actionAsync(
    ReqT req, void (*ackDecoderCB)(Vehicle *vehicle, RecvContainer recvFrame,
                                   UCBRetCodeHandler *ucb),
    void (*userCB)(ErrorCode::ErrCodeType, UserData userData),
    UserData userData, int timeout, int retry_time) {
  if (controlLink) {
    controlLink->sendAsync(OpenProtocolCMD::CMDSet::Control::task, &req,
                           sizeof(req), (void *)ackDecoderCB,
                           allocUCBHandler((void *)userCB, userData), timeout,
                           retry_time);
  } else {
    if (userCB) userCB(ErrorCode::UnifiedErrCode::kErrorSystemError, userData);
  }
}

ErrorCode::ErrCodeType FlightActions::takeoffSync(int timeout) {
  return actionSync(FlightCommand::TAKE_OFF, timeout);
}

void FlightActions::takeoffAsync(
    void (*UserCallBack)(ErrorCode::ErrCodeType retCode, UserData userData),
    UserData userData) {
  this->actionAsync(FlightCommand::TAKE_OFF, commonAckDecoder, UserCallBack,
                    userData);
}

ErrorCode::ErrCodeType FlightActions::forceLandingSync(int timeout) {
  return actionSync(FlightCommand::FORCE_LANDING, timeout);
}

void FlightActions::forceLandingAsync(
    void (*UserCallBack)(ErrorCode::ErrCodeType retCode, UserData userData),
    UserData userData) {
  this->actionAsync(FlightCommand::FORCE_LANDING, commonAckDecoder,
                    UserCallBack, userData);
}

ErrorCode::ErrCodeType FlightActions::forceLandingAvoidGroundSync(int timeout) {
  return actionSync(FlightCommand::FORCE_LANDING_AVOID_GROUND, timeout);
}

void FlightActions::forceLandingAvoidGroundAsync(
    void (*UserCallBack)(ErrorCode::ErrCodeType retCode, UserData userData),
    UserData userData) {
  this->actionAsync(FlightCommand::FORCE_LANDING_AVOID_GROUND, commonAckDecoder,
                    UserCallBack, userData);
}