/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
 * All rights reserved.
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "random-walk-mobility-model.h"
#include "ns3/default-value.h"
#include "ns3/time-default-value.h"
#include "ns3/simulator.h"
#include "ns3/debug.h"
#include <cmath>

NS_DEBUG_COMPONENT_DEFINE ("RandomWalk");

namespace ns3 {

const InterfaceId RandomWalkMobilityModel::iid = 
  MakeInterfaceId ("RandomWalkMobilityModel", MobilityModel::iid);
const ClassId RandomWalkMobilityModel::cid = 
  MakeClassId<RandomWalkMobilityModel, double, double> ("RandomWalkMobilityModel", RandomWalkMobilityModel::iid);


static IntegerDefaultValue<double> g_minSpeed ("RandomWalkMinSpeed", 
					       "Minimum speed used during a random walk", 
					       0.1);
static IntegerDefaultValue<double> g_maxSpeed ("RandomWalkMaxSpeed", 
					       "Maximum speed used during a random walk",
					       0.5);
static EnumDefaultValue<RandomWalkMobilityModelParameters::Mode> g_mode 
  ("RandomWalkMode",
   "The mode indicates the condition used to "
   "change the current speed and direction",
   RandomWalkMobilityModelParameters::MODE_DISTANCE, "Distance",
   RandomWalkMobilityModelParameters::MODE_TIME, "Time",
   0, 0);
static IntegerDefaultValue<double> g_modeDistance ("RandomWalkModeDistance",
						   "Distance to walk before changing direction and speed.",
						   10);
static TimeDefaultValue g_modeTime ("RandomWalkModeTime",
				    "Time to walk before changing direction and speed.",
				    Seconds (1));

RandomWalkMobilityModelParameters::RandomWalkMobilityModelParameters ()
  : m_minSpeed (g_minSpeed.GetValue ()),
    m_maxSpeed (g_maxSpeed.GetValue ()),
    m_mode (g_mode.GetValue ()),
    m_modeDistance (g_modeDistance.GetValue ()),
    m_modeTime (g_modeTime.GetValue ())
{}
bool 
RandomWalkMobilityModelParameters::IsDefault (void) const
{
  if (m_minSpeed != g_minSpeed.GetValue () ||
      m_maxSpeed != g_maxSpeed.GetValue () ||
      m_mode != g_mode.GetValue () ||
      m_modeDistance != g_modeDistance.GetValue () ||
      m_modeTime != g_modeTime.GetValue ())
    {
      return false;
    }
  return true;
}

void
RandomWalkMobilityModelParameters::SetSpeedBounds (double minSpeed, double maxSpeed)
{
  m_minSpeed = minSpeed;
  m_maxSpeed = maxSpeed;
}


UniformVariable RandomWalkMobilityModel::m_randomDirection (0.0, 2*3.141592);

Ptr<RandomWalkMobilityModelParameters> 
RandomWalkMobilityModel::GetDefaultParameters (void)
{
  static Ptr<RandomWalkMobilityModelParameters> parameters = Create<RandomWalkMobilityModelParameters> ();
  if (!parameters->IsDefault ())
    {
      parameters = Create<RandomWalkMobilityModelParameters> ();
    }
  return parameters;
}

RandomWalkMobilityModel::RandomWalkMobilityModel ()
  : m_x (0.0),
    m_y (0.0),
    m_dx (0.0),
    m_dy (0.0),
    m_prevTime (Simulator::Now ()),
    m_parameters (RandomWalkMobilityModel::GetDefaultParameters ())
{
  SetInterfaceId (RandomWalkMobilityModel::iid);
  Reset ();
}

RandomWalkMobilityModel::RandomWalkMobilityModel (double x, double y)
  : m_x (x),
    m_y (y),
    m_dx (0.0),
    m_dy (0.0),
    m_prevTime (Simulator::Now ()),
    m_parameters (RandomWalkMobilityModel::GetDefaultParameters ())
{
  SetInterfaceId (RandomWalkMobilityModel::iid);
  Reset ();
}

void
RandomWalkMobilityModel::Reset (void)
{
  Update ();
  double speed = UniformVariable::GetSingleValue (m_parameters->m_minSpeed, 
						  m_parameters->m_maxSpeed);
  NS_DEBUG ("min="<< m_parameters->m_minSpeed << ", max=" << m_parameters->m_maxSpeed <<
            ", speed=" << speed);
  double direction = m_randomDirection.GetValue ();
  double dx = std::cos (direction) * speed;
  double dy = std::sin (direction) * speed;
  m_dx = dx;
  m_dy = dy;
  Time delay;
  if (m_parameters->m_mode == RandomWalkMobilityModelParameters::MODE_TIME)
    {
      delay = m_parameters->m_modeTime;
    }
  else
    {
      double distance = m_parameters->m_modeDistance;
      delay = Seconds (distance / sqrt (m_dx * m_dx + m_dy * m_dy));
    }
  NotifyCourseChange ();
  NS_DEBUG ("change speed at " << Simulator::Now () << " in " << delay);
  Simulator::Schedule (delay, &RandomWalkMobilityModel::Reset, this);
}

void
RandomWalkMobilityModel::Update (void) const
{
  Time deltaTime = Simulator::Now () - m_prevTime;
  m_prevTime = Simulator::Now ();
  double deltaS = deltaTime.GetSeconds ();
  m_x += m_dx * deltaS;
  m_y += m_dy * deltaS;
}

void
RandomWalkMobilityModel::DoDispose (void)
{
  m_parameters = 0;
  // chain up
  MobilityModel::DoDispose ();
}
Position
RandomWalkMobilityModel::DoGet (void) const
{
  Update ();
  return Position (m_x, m_y, 0.0);
}
void
RandomWalkMobilityModel::DoSet (const Position &position)
{
  bool changed = false;
  if (m_x != position.x || m_y != position.y)
    {
      changed = true;
    }
  m_x = position.x;
  m_y = position.y;
  m_prevTime = Simulator::Now ();
  if (changed)
    {
      NotifyCourseChange ();
    }
}


} // namespace ns3
