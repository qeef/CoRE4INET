//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "CoRE4INET_AVBTrafficSinkApp.h"

//CoRE4INET
#include "CoRE4INET_Defs.h"
#include "CoRE4INET_SRPTable.h"
#include "CoRE4INET_ConfigFunctions.h"

//INET Auto-Generated Messages
#include "EtherFrame_m.h"

namespace CoRE4INET {

Define_Module(AVBTrafficSinkApp);

AVBTrafficSinkApp::AVBTrafficSinkApp()
{
    this->srpTable = NULL;
    this->vlan_id = 0;
    this->streamID = 0;
    updateInterval = 0;
    retryInterval = 0;
}

void AVBTrafficSinkApp::initialize()
{
    TrafficSinkApp::initialize();

    srpTable = check_and_cast_nullable<SRPTable *>(getParentModule()->getSubmodule("srpTable"));
    if (!srpTable)
    {
        throw cRuntimeError("Parent module does not contain a srpTable module");
    }
    srpTable->subscribe("talkerRegistered", this);
    srpTable->subscribe("listenerRegistrationTimeout", this);

    getDisplayString().setTagArg("i2", 0, "status/hourglass");
}

void AVBTrafficSinkApp::receiveSignal(cComponent *src, simsignal_t id, cObject *obj)
{
    Enter_Method_Silent
    ();

    if (id == registerSignal("talkerRegistered"))
    {
        SRPTable::TalkerEntry *tentry = check_and_cast<SRPTable::TalkerEntry*>(obj);

        //If talker for the desired stream, register Listener
        if (tentry->streamId == (unsigned int) par("streamID").longValue()
                && tentry->vlan_id == (unsigned int) par("vlan_id").longValue())
        {
            SRPTable *srpTable = check_and_cast<SRPTable *>(src);

            srpTable->updateListenerWithStreamId(tentry->streamId, this, tentry->vlan_id);
            getDisplayString().setTagArg("i2", 0, "status/hourglass");
            simtime_t updateInterval = par("updateInterval").doubleValue();
            if (updateInterval != 0)
            {
                scheduleAt(simTime() + updateInterval, new cMessage("updateSubscription"));
            }
        }
    }
    else if (id == registerSignal("listenerRegistrationTimeout"))
    {
        SRPTable::ListenerEntry *lentry = check_and_cast<SRPTable::ListenerEntry*>(obj);
        if (lentry->streamId == (unsigned int) par("streamID").longValue()
                && lentry->vlan_id == (unsigned int) par("vlan_id").longValue())
        {
            if (lentry->module == this)
            {
                getDisplayString().setTagArg("i2", 0, "status/hourglass");
                simtime_t retryInterval = par("retryInterval").doubleValue();
                if (retryInterval != 0)
                {
                    scheduleAt(simTime() + retryInterval, new cMessage("retrySubscription"));
                }
            }
        }
    }
}

void AVBTrafficSinkApp::handleMessage(cMessage *msg)
{
    if (msg && msg->isSelfMessage())
    {
        srpTable->updateListenerWithStreamId((unsigned int) par("streamID").longValue(), this,
                (uint16_t) par("vlan_id").longValue());
        getDisplayString().setTagArg("i2", 0, "status/active");
        simtime_t updateInterval = par("updateInterval").doubleValue();
        if (updateInterval > 0)
        {
            scheduleAt(simTime() + updateInterval, msg);
        }
    }
    else
    {
        //Received an EtherFrame so the source seems to be active
        if (dynamic_cast<EtherFrame*>(msg))
        {
            getDisplayString().setTagArg("i2", 0, "status/active");
        }
        TrafficSinkApp::handleMessage(msg);
    }
}

void AVBTrafficSinkApp::handleParameterChange(const char* parname)
{
    if (!parname || !strcmp(parname, "updateInterval"))
    {
        this->updateInterval = parameterDoubleCheckRange(par("updateInterval"), 0, DBL_MAX);
    }
    if (!parname || !strcmp(parname, "retryInterval"))
    {
        this->retryInterval = parameterDoubleCheckRange(par("retryInterval"), 0, DBL_MAX);
    }
    if (!parname || !strcmp(parname, "streamID"))
    {
        this->streamID = parameterULongCheckRange(par("streamID"), 0, (unsigned long)MAX_STREAM_ID);
    }
    if (!parname || !strcmp(parname, "vlan_id"))
    {
        this->vlan_id = parameterLongCheckRange(par("vlan_id"), 0, MAX_VLAN_ID);
    }
}

} //namespace
