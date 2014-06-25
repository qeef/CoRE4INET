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

#ifndef __CORE4INET_SCHEDULED_H_
#define __CORE4INET_SCHEDULED_H_

//CoRE4INET
#include "CoRE4INET_Timed.h"
#include "CoRE4INET_Period.h"

namespace CoRE4INET {

/**
 * @brief Base class for a module that uses a scheduler to schedule actions at action points
 *
 * @sa Timed
 *
 * @author Till Steinbach
 */
class Scheduled : public Timed
{
    protected:
        /**
         * The period related to the actions of this module default is period[0]
         */
        Period *period;

    public:
        /**
         * @brief Initialization of the module sets period[0] when parameter period is empty
         */
        void initialize();

        /**
         * returns pointer to the configured period
         */
        Period* getPeriod() const;
};

}
#endif /* __CORE4INET_SCHEDULED_H_ */