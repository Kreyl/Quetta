/*
 * EvtMsgIDs.h
 *
 *  Created on: 21 ���. 2017 �.
 *      Author: Kreyl
 */

#ifndef EVTMSGIDS_H__
#define EVTMSGIDS_H__

enum EvtMsgId_t {
    evtIdNone = 0, // Always

    evtIdShellCmdRcvd, // UART
    evtIdEverySecond,

    // Usb
    evtIdUsbConnect,
    evtIdUsbDisconnect,
    evtIdUsbReady,
    evtIdUsbCmdRcvd,
    evtIdUsbInDone,
    evtIdUsbOutDone,

};

#endif //EVTMSGIDS_H__
