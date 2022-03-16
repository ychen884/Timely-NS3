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
 *
 *
 *
 */

#include "tcp-socket-state.h"

#include "tcp_cc_timely.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("TCPCCTIMELY");
    NS_OBJECT_ENSURE_REGISTERED(TCPCCTIMELY);

    TypeId
    TCPCCTIMELY::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::TCPCCTIMELY")
                                /**parameters we need for timely:
                                      1. EWMA weight parameter
                                      2. Lower threshold
                                      3. Higher threshold
                                      4. addtive increment step
                                      5. multiplicative decrement factor
                                      6. HAI mode counter
                                      */
                                .SetParent<TcpNewReno>()
                                .AddConstructor<TCPCCTIMELY>()
                                .SetGroupName("Internet")
                                .AddAttribute("Alpha", "EMWA",
                                              DoubleValue(0.1),
                                              MakeDoubleAccessor(&TCPCCTIMELY::m_alpha),
                                              MakeDoubleChecker<float>())
                                .AddAttribute("Lth", "T low",
                                              DoubleValue(250),
                                              MakeDoubleAccessor(&TCPCCTIMELY::m_lowerth),
                                              MakeDoubleChecker<double>())
                                .AddAttribute("Hth", "T high",
                                              DoubleValue(4000),
                                              MakeDoubleAccessor(&TCPCCTIMELY::m_higherth),
                                              MakeDoubleChecker<double>())
                                .AddAttribute("AI", "additive increase",
                                              DoubleValue(4),
                                              MakeDoubleAccessor(&TCPCCTIMELY::m_ai),
                                              MakeDoubleChecker<float>())

                                .AddAttribute("MD", "multiplicatively decrease",
                                              DoubleValue(0.05),
                                              MakeDoubleAccessor(&TCPCCTIMELY::m_md),
                                              MakeDoubleChecker<float>())
                                .AddAttribute("N_hai", "HAI parameter",
                                              UintegerValue(5),
                                              MakeUintegerAccessor(&TCPCCTIMELY::m_N),
                                              MakeUintegerChecker<uint32_t>())
                                .AddAttribute("initial_rate", "initial sending rate",
                                              DoubleValue(5),
                                              MakeDoubleAccessor(&TCPCCTIMELY::m_sendingRate),
                                              MakeDoubleChecker<float>());

        return tid;
    }
    //        float m_alpha; //!< EWMA weight parameter
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

    TCPCCTIMELY::TCPCCTIMELY(void)
        : TcpNewReno(),
          m_alpha(0.1),
          m_lowerth(500),
          m_higherth(5500),

          m_ai(1.0),
          m_md(0.05),
          m_N(5),
          m_prevRTT(-1),
          m_newRTTDiff(0),
          m_sendingRate(5),
          m_eventcount(0),
          m_baseRtt(MAXFLOAT),
          m_minRtt(MAXFLOAT),
          m_cntRtt(0),
          m_doingTIMELYNow(true),
          m_begSndNxt(0),
          m_lastt(0),
          m_samplertt(0)

    {
        NS_LOG_FUNCTION(this);
    }

    TCPCCTIMELY::TCPCCTIMELY(const TCPCCTIMELY &sock)
        : TcpNewReno(sock),
          m_alpha(sock.m_alpha),
          m_lowerth(sock.m_lowerth),
          m_higherth(sock.m_higherth),

          m_ai(sock.m_ai),
          m_md(sock.m_md),
          m_N(sock.m_N),

          m_prevRTT(-1),
          m_newRTTDiff(sock.m_newRTTDiff),
          m_sendingRate(5),
          m_eventcount(sock.m_eventcount),
          m_baseRtt(MAXFLOAT),
          m_minRtt(MAXFLOAT),

          m_cntRtt(sock.m_cntRtt),
          m_doingTIMELYNow(true),
          m_begSndNxt(0),
          m_lastt(0),
          m_samplertt(0)

    {
        NS_LOG_FUNCTION(this);
    }

    TCPCCTIMELY::~TCPCCTIMELY(void)
    {
        NS_LOG_FUNCTION(this);
    }

    Ptr<TcpCongestionOps>
    TCPCCTIMELY::Fork(void)
    {
        return CopyObject<TCPCCTIMELY>(this);
    }

    void
    TCPCCTIMELY::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                           const Time &rtt)
    {
        // LogComponentEnable("TCPCCTIMELY", LOG_LEVEL_DEBUG);
        //  A completion event is generated upon receiving an ACK for a segment of data and includes the ACK receive time.
        NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);
        // Update RTT counter
        m_cntRtt++;
        if (rtt.IsZero())
        {
            return;
        }
        if (!tcp_rtt_stat.IsNull())
            tcp_rtt_stat(rtt.GetMicroSeconds());

        double acked_rtt = rtt.GetMicroSeconds();
        // update baseRTT, not used for timely
        m_baseRtt = std::min(m_baseRtt, acked_rtt);

        this->m_minRtt = std::min(this->m_minRtt, acked_rtt);
        double current_time = ns3::Simulator::Now().GetMicroSeconds(); // compute rate at most once per RTT

        if (current_time - this->m_lastt < this->m_samplertt)
        {
            // we do not want to overweigh the new information
            // std::cout << "skipping..." << std::endl;
            return;
        }
        this->m_lastt = current_time;
        NS_LOG_DEBUG("Updated m_baseRtt = " << m_baseRtt);

        if (this->m_samplertt == 0)
        {
            this->m_samplertt = acked_rtt;
        }
        else
        {
            this->m_samplertt = acked_rtt * (1 - this->m_alpha) + this->m_alpha * this->m_samplertt;
        }

        double new_rtt = acked_rtt;
        if (m_prevRTT == -1)
        {
            m_prevRTT = acked_rtt;
        }

        double prev_rtt = this->m_prevRTT;

        // update rtt parameters, calculate normalized gradient
        double new_rtt_diff = new_rtt - prev_rtt;
        this->m_prevRTT = new_rtt;

        this->m_newRTTDiff = (1 - this->m_alpha) * this->m_newRTTDiff + this->m_alpha * new_rtt_diff;


        double normalized_gradient = this->m_newRTTDiff / this->m_minRtt;
        std::cout << "G," << normalized_gradient << "," << ns3::Simulator::Now().GetMicroSeconds() << std::endl;

        if (new_rtt < this->m_lowerth)
        {
            // int prev = tcb->m_cWnd;
            NS_LOG_DEBUG("TOO LOW");
            // rate ← rate + δ ;
            this->m_sendingRate = this->m_sendingRate + this->m_ai;
            this->m_eventcount = 0;
            tcb->m_cWnd = this->m_sendingRate * tcb->m_segmentSize;
            return;
        }
        else if (new_rtt > this->m_higherth)
        {
            // int prev = tcb->m_cWnd;
            NS_LOG_DEBUG("TOO High");
            // rate ← rate · ( 1 - β · ( 1 - Thigh/new_rtt))

            this->m_sendingRate = this->m_sendingRate * (1 - this->m_md * (1 - this->m_higherth / new_rtt));
            this->m_eventcount = 0;
            tcb->m_cWnd = this->m_sendingRate * tcb->m_segmentSize;

            return;
        }
        // int prev = tcb->m_cWnd;

        // HAI mode control based on consecutive events
        if (normalized_gradient <= 0)
        {
            // negative gradient
            if ((this->m_eventcount + 1) >= 5)
            {
                // HAI mode
                int N = this->m_N;
                // sending rate is increased
                this->m_sendingRate = this->m_sendingRate + N * this->m_ai;
                // suppose we go back to normal mode
                this->m_eventcount = 0;
            }
            else
            {
                this->m_sendingRate = this->m_sendingRate + this->m_ai;
                this->m_eventcount += 1;
            }
        }
        else
        {
            // we only care consecutive events
            this->m_eventcount = 0;
            // positive gradient
            // rate ← rate · (1 - β · normalized_gradient)
            this->m_sendingRate = this->m_sendingRate * (1 - this->m_md * normalized_gradient);
        }

        // adjust window size based on sending rate
        tcb->m_cWnd = this->m_sendingRate * tcb->m_segmentSize;
    }

    void
    TCPCCTIMELY::EnableTIMELY(Ptr<TcpSocketState> tcb)
    {
        NS_LOG_FUNCTION(this << tcb);

        m_doingTIMELYNow = true;
        m_begSndNxt = tcb->m_nextTxSequence;
        m_cntRtt = 0;
        m_minRtt = MAXFLOAT;
        // not sure if we need to reset rate
    }

    void
    TCPCCTIMELY::DisableTIMELY()
    {
        NS_LOG_FUNCTION(this);

        m_doingTIMELYNow = false;
    }

    void
    TCPCCTIMELY::CongestionStateSet(Ptr<TcpSocketState> tcb,
                                    const TcpSocketState::TcpCongState_t newState)
    {
        NS_LOG_FUNCTION(this << tcb << newState);
        if (newState == TcpSocketState::CA_OPEN)
        {
            EnableTIMELY(tcb);
        }
        else
        {
            DisableTIMELY();
        }
    }

    void
    TCPCCTIMELY::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
    {
        // no need to change window, since all handled in the PktsAcked
    }

    std::string
    TCPCCTIMELY::GetName() const
    {
        return "TCPCCTIMELY";
    }

    uint32_t
    TCPCCTIMELY::GetSsThresh(Ptr<const TcpSocketState> tcb,
                             uint32_t bytesInFlight)
    {
        NS_LOG_FUNCTION(this << tcb << bytesInFlight);
        // default slow start
        return std::max(std::min(tcb->m_ssThresh.Get(), tcb->m_cWnd.Get() - tcb->m_segmentSize), 2 * tcb->m_segmentSize);
    }

} // namespace ns3
