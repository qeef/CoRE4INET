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
#ifndef CORE4INET_SRPTABLE_H_
#define CORE4INET_SRPTABLE_H_

//Std
#include <map>
#include <unordered_map>

//CoRE4INET
#include "core4inet/base/CoRE4INET_Defs.h"
#include "core4inet/base/avb/AVBDefs.h"

//INET
#include "inet/linklayer/common/MACAddress.h"

//Auto-generated Messages
#include "core4inet/base/avb/AVBDefs_m.h"

namespace CoRE4INET {

/**
 * @brief This module handles the mapping between ports and MAC addresses.
 *
 * See the NED definition for details.
 *
 * @author Till Steinbach, Philipp Meyer
 */
class SRPTable : public virtual cSimpleModule
{
    public:
        /**
         * @brief Entry for Talker
         */
        class TalkerEntry : public cObject
        {
            public:
                uint64_t streamId;          // Stream ID
                SR_CLASS srClass;           // Stream Reservation Class
                inet::MACAddress address;   // The talkers address
                cModule *module;            // Input port or module
                size_t framesize;           // Frame size in byte
                uint16_t intervalFrames;    // interval frames
                uint16_t vlan_id;           // VLAN identifier
                simtime_t insertionTime;    // Arrival time of SRP entry

                TalkerEntry();
                TalkerEntry(uint64_t new_streamId, SR_CLASS new_srClass, inet::MACAddress new_address,
                        cModule *new_module, size_t new_framesize, unsigned short new_intervalFrames,
                        unsigned short new_vlan_id, simtime_t new_insertionTime);
                virtual ~TalkerEntry();

        };
        friend std::ostream& operator<<(std::ostream& os, const TalkerEntry& entry);

        /**
         * @brief Entry for Listener
         */
        class ListenerEntry : public cObject
        {
            public:
                uint64_t streamId;          // Stream ID
                cModule *module;            // Listener port or module
                uint16_t vlan_id;           // VLAN identifier
                simtime_t insertionTime;    // Arrival time of SRP entry

                ListenerEntry();
                ListenerEntry(uint64_t new_streamId, cModule *new_module, unsigned short new_vlan_id,
                        simtime_t new_insertionTime);
                virtual ~ListenerEntry();

        };
        friend std::ostream& operator<<(std::ostream& os, const ListenerEntry& entry);

        typedef std::unordered_map<uint64_t, TalkerEntry*> TalkerTable;
        typedef std::unordered_map<cModule*, ListenerEntry*> ListenerList;
        typedef std::unordered_map<uint64_t, ListenerList> ListenerTable;

    private:
        /**
         * map of talker entries for stream id
         */
        std::unordered_map<unsigned int, TalkerTable> talkerTables;

        /**
         * map of listener entries for stream id
         */
        std::unordered_map<unsigned int, ListenerTable> listenerTables;

        /**
         * Time when next entry is aging
         */
        simtime_t nextAging;

    protected:

        /**
         *  @brief Initialization, registers WATCH and updates display string
         */
        virtual void initialize() override;

        /**
         *  @brief Table does not receive messages, throws cRuntimeError when handleMessage is called
         */
        virtual void handleMessage(cMessage *msg) override __attribute__ ((noreturn));

    public:
        /**
         *  @brief Constructor
         */
        SRPTable();

        /**
         *  @brief Destructor
         */
        virtual ~SRPTable() override;

    public:
        /**
         * @brief For a known talker address and V-TAG it finds out the streamId
         *
         * @param talkerAddress address
         * @param vid VLAN ID
         * @return streamId related to talkerAddress
         */
        virtual uint64_t getStreamIdForTalkerAddress(const inet::MACAddress &talkerAddress, uint16_t vid =
                VLAN_ID_DEFAULT);

        /**
         * @brief For a known talker address and V-TAG it finds out the SR-Class of the Stream
         *
         * @param talkerAddress address
         * @param vid VLAN ID
         * @return SR-Class related to the Stream of the talkerAddress
         */
        virtual SR_CLASS getSrClassForTalkerAddress(const inet::MACAddress &talkerAddress, uint16_t vid =
                VLAN_ID_DEFAULT);

        /**
         * @brief For a known streamId and V-TAG it finds out the port where relay component should deliver the message
         *
         * @param streamId streamId
         * @param vid VLAN ID
         * @return listeners for the stream
         */
        virtual std::list<cModule*> getListenersForStreamId(uint64_t streamId, uint16_t vid = VLAN_ID_DEFAULT);

        /**
         * @brief For a known talker address and V-TAG it finds out the port where relay component should deliver the message
         *
         * @param talkerAddress address
         * @param vid VLAN ID
         * @return listeners for the stream
         */
        virtual std::list<cModule*> getListenersForTalkerAddress(const inet::MACAddress &talkerAddress, uint16_t vid =
                VLAN_ID_DEFAULT);

        /**
         * @brief Retrieve the module a message with a given streamId will come from (required for listener ready messages)
         *
         * @param streamId the streams id
         * @param vid VLAN ID
         * @return Output port for address, or -1 if unknown.
         */
        virtual cModule* getTalkerForStreamId(uint64_t streamId, uint16_t vid = VLAN_ID_DEFAULT);

        /**
         * @brief Retrieve the required bandwidth for a module with registered listeners
         *
         * @param module the module registered as listener
         * @return bandwidth in bps
         */
        virtual unsigned long getBandwidthForModule(const cModule *module);

        /**
         * @brief Retrieve the required bandwidth for a module with registered listeners per SR-Class
         *
         * @param module the module registered as listener
         * @param srClass the SR-Class (A or B)
         * @return bandwidth in bps
         */
        virtual unsigned long getBandwidthForModuleAndSRClass(const cModule *module, SR_CLASS srClass);

        /**
         * @brief Retrieve the required bandwidth for a stream
         *
         * @param streamId the streams id
         * @param vid VLAN ID
         * @return bandwidth in bps
         */
        virtual unsigned long getBandwidthForStream(uint64_t streamId, uint16_t vid = VLAN_ID_DEFAULT);

        /**
         * @brief Register a new streamId at talkerTable.
         * @return True if refreshed. False if it is new.
         */
        virtual bool updateTalkerWithStreamId(uint64_t streamId, cModule *module, const inet::MACAddress address,
                SR_CLASS srClass = SR_CLASS::A, size_t framesize = 0, uint16_t intervalFrames = 0, uint16_t vid =
                        VLAN_ID_DEFAULT);

        /**
         * @brief Unregister a streamId at talkerTable.
         * @return True if removed. False if not registered.
         */
        virtual bool removeTalkerWithStreamId(uint64_t streamId, cModule *module, const inet::MACAddress address,
                uint16_t vid = VLAN_ID_DEFAULT);

        /**
         * @brief Register a new streamId at listenerTable.
         * @return True if refreshed. False if it is new.
         */
        virtual bool updateListenerWithStreamId(uint64_t streamId, cModule *module, uint16_t vid = VLAN_ID_DEFAULT);

        /**
         * @brief Unregister a streamId at listenerTable.
         * @return True if removed. False if not registered.
         */
        virtual bool removeListenerWithStreamId(uint64_t streamId, cModule *module, uint16_t vid = VLAN_ID_DEFAULT);

        /**
         *  @brief Prints cached data
         */
        virtual void printState();

        /**
         * @brief For lifecycle: clears all entries from the table.
         */
        virtual void clear();

        /**
         * @brief Remove entries that were not updated during agingTime if necessary
         */
        void removeAgedEntriesIfNeeded();

    protected:
        /**
         * @brief Remove entries that were not updated during agingTime
         */
        void removeAgedEntries();

        /**
         * @brief updates the displaystring of talkers and listeners
         */
        void updateDisplayString();
        /**
         * @brief get the number of registered talkers
         *
         * @return number of talkers
         */
        size_t getNumTalkerEntries();

        /**
         * @brief get the number of registered listeners
         *
         * @return number of listeners
         */
        size_t getNumListenerEntries();
};

}
#endif
