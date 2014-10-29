//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "CoRE4INET_SRProtocol.h"

//Std
#include <algorithm>
//CoRE4INET
#include "CoRE4INET_AVBDefs.h"
#include "ExtendedIeee802Ctrl_m.h"
//Auto-generated Messages
#include "SRPFrame_m.h"

#define ETHERAPP_BUFFER_SAP  0xe1

namespace CoRE4INET {

Define_Module(SRProtocol);

void SRProtocol::initialize()
{
    srpTable = (SRPTable *) getParentModule()->getSubmodule("srpTable");
    if (!srpTable)
    {
        throw cRuntimeError("srpTable module required for stream reservation");
    }
    srpTable->subscribe("talkerRegistered", this);
    srpTable->subscribe("listenerRegistered", this);
    srpTable->subscribe("listenerUpdated", this);
}

void SRProtocol::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("in"))
    {
        Ieee802Ctrl *etherctrl = dynamic_cast<Ieee802Ctrl *>(msg->removeControlInfo());
        if (!etherctrl)
        {
            error("packet `%s' from lower layer received without Ieee802Ctrl", msg->getName());
        }
        int arrivedOn = etherctrl->getSwitchPort();
        cModule *port = getParentModule()->getSubmodule("phy", arrivedOn);

        if (TalkerAdvertise* talkerAdvertise = dynamic_cast<TalkerAdvertise*>(msg))
        {
            SR_CLASS srClass;
            if(talkerAdvertise->getPriorityAndRank() == PRIOANDRANK_SRCLASSA) srClass = SR_CLASS_A;
            if(talkerAdvertise->getPriorityAndRank() == PRIOANDRANK_SRCLASSB) srClass = SR_CLASS_B;
            srpTable->updateTalkerWithStreamId(talkerAdvertise->getStreamID(), port, talkerAdvertise->getDestination_address(), srClass,
                    talkerAdvertise->getMaxFrameSize(), talkerAdvertise->getMaxIntervalFrames(), talkerAdvertise->getVlan_identifier());
        }
        else if (ListenerReady* listenerReady = dynamic_cast<ListenerReady*>(msg))
        {
            bool update = false;
            //TODO Minor: try to get VLAN.
            std::list<cModule*> listeners = srpTable->getListenersForStreamId(listenerReady->getStreamID(), listenerReady->getVlan_identifier());
            //Is this a new or updated stream
            if (std::find(listeners.begin(), listeners.end(), port) != listeners.end())
            {
                update = true;
            }

            unsigned long utilizedBandwidth = srpTable->getBandwidthForModule(port);
            //Add Higher Priority Bandwidth
            utilizedBandwidth += port->getSubmodule("shaper")->par("AVBHigherPriorityBandwidth").longValue();
            //TODO Minor: try to get VLAN.
            unsigned long requiredBandwidth = srpTable->getBandwidthForStream(listenerReady->getStreamID(), listenerReady->getVlan_identifier());

            cGate *physOutGate = port->getSubmodule("mac")->gate("phys$o");
            cChannel *outChannel = physOutGate->findTransmissionChannel();

            unsigned long totalBandwidth = outChannel->getNominalDatarate();

            double reservableBandwidth = par("reservableBandwidth").doubleValue();

            if (update || ((utilizedBandwidth + requiredBandwidth) <= (totalBandwidth * reservableBandwidth)))
            {
                //TODO Minor: try to get VLAN.
                srpTable->updateListenerWithStreamId(listenerReady->getStreamID(), port, listenerReady->getVlan_identifier());
                if(!update)
                {
                    EV_DETAIL << "Listener for stream " << listenerReady->getStreamID() << " registered on port "
                            << port->getFullName() << ". Utilized Bandwidth: "
                            << (utilizedBandwidth + requiredBandwidth) / (double) 1000000 << " of "
                            << (totalBandwidth * reservableBandwidth) / (double) 1000000 << " reservable Bandwidth of "
                            << totalBandwidth / (double) 1000000 << " MBit/s.";
                }
            }
            else
            {
                EV_DETAIL << "Listener for stream " << listenerReady->getStreamID()
                        << " could not be registered on port " << port->getFullName() << ". Required bandwidth: "
                        << requiredBandwidth / (double) 1000000 << "MBit/s, remaining bandwidth "
                        << ((totalBandwidth * reservableBandwidth) - utilizedBandwidth) / (double) 1000000 << "MBit/s.";
                SRPFrame *srp;
                if (srpTable->getListenersForStreamId(listenerReady->getStreamID(), 0).size() > 0)
                {
                    bubble("Listener Ready Failed!");
                    srp = new ListenerReadyFailed("Listener Ready Failed", IEEE802CTRL_DATA);
                }
                else
                {
                    bubble("Listener Failed!");
                    srp = new ListenerAskingFailed("Listener Failed", IEEE802CTRL_DATA);
                }
                srp->setStreamID(listenerReady->getStreamID());

                ExtendedIeee802Ctrl *etherctrl = new ExtendedIeee802Ctrl();
                etherctrl->setEtherType(MSRP_ETHERTYPE);
                etherctrl->setDest(SRP_ADDRESS);
                //TODO Minor: try to get VLAN.
                cModule* talker = srpTable->getTalkerForStreamId(listenerReady->getStreamID(), listenerReady->getVlan_identifier());
                if (talker && talker->isName("phy"))
                {
                    etherctrl->setSwitchPort(talker->getIndex());
                    srp->setControlInfo(etherctrl);
                    send(srp, gate("out"));
                }
            }
        }
        else if (ListenerAskingFailed* listenerFailed = dynamic_cast<ListenerAskingFailed*>(msg))
        {
            bubble("Listener Failed!");
            ExtendedIeee802Ctrl *etherctrl = new ExtendedIeee802Ctrl();
            etherctrl->setEtherType(MSRP_ETHERTYPE);
            etherctrl->setDest(SRP_ADDRESS);
            //TODO Minor: try to get VLAN.
            cModule* talker = srpTable->getTalkerForStreamId(listenerFailed->getStreamID(), listenerFailed->getVlan_identifier());
            if (talker && talker->isName("phy"))
            {
                etherctrl->setSwitchPort(talker->getIndex());
                //Necessary because controlInfo is not duplicated
                ListenerAskingFailed* listenerFailedCopy = listenerFailed->dup();
                listenerFailedCopy->setControlInfo(etherctrl);
                send(listenerFailedCopy, gate("out"));
            }
        }
        delete etherctrl;
    }
    delete msg;
}

void SRProtocol::receiveSignal(cComponent *src, simsignal_t id, cObject *obj)
{
    Enter_Method_Silent
    ();
    if (id == registerSignal("talkerRegistered"))
    {
        SRPTable::TalkerEntry *tentry = (SRPTable::TalkerEntry*) obj;

        TalkerAdvertise *talkerAdvertise = new TalkerAdvertise("Talker Advertise", IEEE802CTRL_DATA);
        //talkerAdvertise->setStreamDA(tentry->address);
        talkerAdvertise->setStreamID(tentry->streamId);
        talkerAdvertise->setMaxFrameSize(tentry->framesize);
        talkerAdvertise->setMaxIntervalFrames(tentry->intervalFrames);
        talkerAdvertise->setDestination_address(tentry->address);
        talkerAdvertise->setVlan_identifier(tentry->vlan_id);
        if(tentry->srClass == SR_CLASS_A) talkerAdvertise->setPriorityAndRank(PRIOANDRANK_SRCLASSA);
        if(tentry->srClass == SR_CLASS_B) talkerAdvertise->setPriorityAndRank(PRIOANDRANK_SRCLASSB);

        ExtendedIeee802Ctrl *etherctrl = new ExtendedIeee802Ctrl();
        etherctrl->setEtherType(MSRP_ETHERTYPE);
        etherctrl->setDest(SRP_ADDRESS);
        etherctrl->setSwitchPort(SWITCH_PORT_BROADCAST);
        talkerAdvertise->setControlInfo(etherctrl);

        //If talker was received from phy we have to exclude the incoming port
        if (strcmp(tentry->module->getName(), "phy") == 0)
        {
            etherctrl->setNotSwitchPort(tentry->module->getIndex());
        }

        send(talkerAdvertise, gate("out"));
    }
    else if (id == registerSignal("listenerRegistered") || id == registerSignal("listenerUpdated"))
    {
        SRPTable::ListenerEntry *lentry = (SRPTable::ListenerEntry*) obj;

        //Get Talker Port
        SRPTable *srpTable = (SRPTable *) src;
        cModule* talker = srpTable->getTalkerForStreamId(lentry->streamId, lentry->vlan_id);
        //Send listener ready only when talker is not a local application
        if (talker && talker->isName("phy"))
        {
            ListenerReady *listenerReady = new ListenerReady("Listener Ready", IEEE802CTRL_DATA);
            listenerReady->setStreamID(lentry->streamId);
            listenerReady->setVlan_identifier(lentry->vlan_id);

            ExtendedIeee802Ctrl *etherctrl = new ExtendedIeee802Ctrl();
            etherctrl->setEtherType(MSRP_ETHERTYPE);
            etherctrl->setDest(SRP_ADDRESS);
            etherctrl->setSwitchPort(talker->getIndex());
            listenerReady->setControlInfo(etherctrl);

            send(listenerReady, gate("out"));
        }

    }
}

} /* namespace CoRE4INET */
