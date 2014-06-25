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

#ifndef __CORE4INET_CTAPPLICATIONBASE_H_
#define __CORE4INET_CTAPPLICATIONBASE_H_

//Std
#if __cplusplus >= 201103L
#include <unordered_map>
using namespace std;
#else
#include <tr1/unordered_map>
using namespace std::tr1;
#endif

//CoRE4INET
#include "CoRE4INET_ApplicationBase.h"
#include "CoRE4INET_CTBuffer.h"

namespace CoRE4INET {

/**
 * @brief Base class for a real-time Ethernet-Application.
 *
 * Containes the mapping to Buffers and the ability to execute Callbacks
 *
 * @sa Buffer, Callback
 *
 * @ingroup Applications
 *
 * @author Till Steinbach
 */
class CTApplicationBase : public virtual ApplicationBase
{
    protected:
        /**
         * @brief Map of critical traffic identifies with their
         * corresponding Buffers.
         */
        unordered_map<uint16_t, std::list<CTBuffer*> > ctbuffers;
    public:
        /**
         * @brief Initialization of the module.
         */
        virtual void initialize();
        /**
         * @brief resets the bag on incoming RC-Frames (on RCin)
         *
         * This method should be called from subclasses unless the module
         * resets the bag on its own.
         *
         * @param msg Parameter must be forwarded from subclass
         */
        virtual void handleMessage(cMessage *msg);
        /**
         * @brief Indicates a parameter has changed.
         *
         * @param parname Name of the changed parameter or NULL if multiple parameter changed.
         */
        virtual void handleParameterChange(const char* parname);

};

} //namespace

#endif