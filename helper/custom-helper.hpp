/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
 *               2009,2010 Contributors
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
 * Contributors: Thomas Waldecker <twaldecker@rocketmail.com>
 *               Martín Giachino <martin.giachino@gmail.com>
 */
#ifndef CUSTOM_HELPER_H
#define CUSTOM_HELPER_H

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/lte-module.h"

namespace ns3 {

class CustomHelper
{
public:
  CustomHelper();

  void Install(Ptr<Node> node) const;

private:
  // reusable helpers
  Ptr<LteHelper> m_lteHelper;
  Ptr<PointToPointEpcHelper>  m_epcHelper;
  Ptr<LteSidelinkHelper> m_proseHelper;

  Ptr<Object> m_uplinkPathlossModel;
  Ptr<LteSlUeRrc> m_ueSidelinkConfiguration;
  Ptr<LteSlTft> m_tft;

  InternetStackHelper m_internetHelper;
  Ipv4StaticRoutingHelper m_ipv4RoutingHelper;
};

} // namespace ns3

#endif /* CUSTOM_HELPER_H */
