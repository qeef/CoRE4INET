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

#include "TocApp.h"
#include "TicToc_m.h"
#include "TTEScheduler.h"
#include "RCFrame_m.h"
#include "TTFrame_m.h"

namespace CoRE4INET {

Define_Module(TocApp);

void TocApp::initialize()
{
    ApplicationBase::initialize();
}

void TocApp::handleMessage(cMessage *msg)
{
    ApplicationBase::handleMessage(msg);

    if (msg->arrivedOn("TTin"))
    {
        TTFrame *ttframe = dynamic_cast<TTFrame*>(msg);
        Tic *tic = dynamic_cast<Tic*>(ttframe->decapsulate());
        delete msg;
        bubble(tic->getRequest());
        par("counter").setLongValue((long) tic->getCount());

        Toc *toc = new Toc();
        toc->setRoundtrip_start(tic->getRoundtrip_start());
        toc->setCount(tic->getCount() + 1);
        delete tic;

        CTFrame *frame = new RCFrame("Toc");
        frame->setCtID((uint16_t) par("ct_id").longValue());
        frame->encapsulate(toc);

        EV_DETAIL << "Answering Tic Message with Toc Message\n";
        std::list<CTBuffer*> buffer = ctbuffers[frame->getCtID()];
        for (std::list<CTBuffer*>::iterator buf = buffer.begin(); buf != buffer.end(); buf++)
        {
            Incoming* in = dynamic_cast<Incoming *>((*buf)->gate("in")->getPathStartGate()->getOwner());
            sendDirect(frame->dup(), in->gate("in"));
        }
        delete frame;
    }
    else
    {
        delete msg;
    }
}

}
