/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * modified from tcp-vegas by yizhou
 * ychen884@wisc.edu
 * This is the source code of implemented timely
 */

#ifndef TCPCCTIMELY_H
#define TCPCCTIMELY_H

#include "tcp-congestion-ops.h"

namespace ns3
{

    class TcpSocketState;

    /**
     * \ingroup congestionOps
     *
     * \brief An implementation of TIMELY data center congestion control
     *
     *
     *
     * More information: https://conferences.sigcomm.org/sigcomm/2015/pdf/papers/p537.pdf
     */

    class TCPCCTIMELY : public TcpNewReno
    {
    public:
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId(void);

        /**
         * Create an unbound tcp socket.
         */
        TCPCCTIMELY(void);

        /**
         * \brief Copy constructor
         * \param sock the object to copy
         */
        TCPCCTIMELY(const TCPCCTIMELY &sock);
        virtual ~TCPCCTIMELY(void);

        virtual std::string GetName() const;

        /**
         * \brief Compute RTTs needed to execute Vegas algorithm
         *
         * The function filters RTT samples from the last RTT to find
         * the current smallest propagation delay + queueing delay (minRtt).
         * We take the minimum to avoid the effects of delayed ACKs.
         *
         * The function also min-filters all RTT measurements seen to find the
         * propagation delay (baseRtt).
         *
         * \param tcb internal congestion state
         * \param segmentsAcked count of segments ACKed
         * \param rtt last RTT
         *
         */
        virtual void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                               const Time &rtt);

        /**
         * \brief Enable/disable Vegas algorithm depending on the congestion state
         *
         * We only start a Vegas cycle when we are in normal congestion state (CA_OPEN state).
         *
         * \param tcb internal congestion state
         * \param newState new congestion state to which the TCP is going to switch
         */
        virtual void CongestionStateSet(Ptr<TcpSocketState> tcb,
                                        const TcpSocketState::TcpCongState_t newState);

        /**
         * \brief Adjust cwnd following Vegas linear increase/decrease algorithm
         *
         * \param tcb internal congestion state
         * \param segmentsAcked count of segments ACKed
         */
        virtual void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

        /**
         * \brief Get slow start threshold following Vegas principle
         *
         * \param tcb internal congestion state
         * \param bytesInFlight bytes in flight
         *
         * \return the slow start threshold value
         */
        virtual uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb,
                                     uint32_t bytesInFlight);

        virtual Ptr<TcpCongestionOps> Fork();

    protected:
    private:
        /**
         * \brief Enable Vegas algorithm to start taking Vegas samples
         *
         * Vegas algorithm is enabled in the following situations:
         * 1. at the establishment of a connection
         * 2. after an RTO
         * 3. after fast recovery
         * 4. when an idle connection is restarted
         *
         * \param tcb internal congestion state
         */
        void EnableTIMELY(Ptr<TcpSocketState> tcb);

        /**
         * \brief Stop taking Vegas samples
         */
        void DisableTIMELY();

    private:
        // algorithm parameters
        float m_alpha; //!< EWMA weight parameter
        double m_lowerth;
        double m_higherth;
        float m_ai;   // AIMD
        float m_md;   // AIMD
        uint32_t m_N; // HAI count
        double m_newRTT;
        double m_prevRTT;
        double m_newRTTDiff;
        double m_sendingRate;
        uint32_t m_eventcount;

        // general
        double m_baseRtt;      //!< Minimum of all TIMELY RTT measurements seen during connection
        double m_minRtt;       //!< Minimum of all RTT measurements within last RTT
        uint32_t m_cntRtt;     //!< Number of RTT measurements during last RTT
        bool m_doingTIMELYNow; //!< If true, do TIMELY for this RTT

        

        SequenceNumber32 m_begSndNxt; //!< Right edge during last RTT
        double m_lastt;
        double m_samplertt;
    };

} // namespace ns3

#endif // TCPCCTIMELY_H
