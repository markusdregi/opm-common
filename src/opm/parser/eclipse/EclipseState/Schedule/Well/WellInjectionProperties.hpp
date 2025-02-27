/*
  Copyright 2014 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef WELLINJECTIONPROPERTIES_HPP_HEADER_INCLUDED
#define WELLINJECTIONPROPERTIES_HPP_HEADER_INCLUDED

#include <iosfwd>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/InjectionControls.hpp>

namespace Opm {

    class DeckRecord;
    class UnitSystem;
    class SummaryState;

    struct WellInjectionProperties {
        double  surfaceInjectionRate;
        double  reservoirInjectionRate;
        double  temperature;
        double  BHPLimit;
        double  THPLimit;
        double  BHPH;
        double  THPH;
        int     VFPTableNumber;
        bool    predictionMode;
        int     injectionControls;
        WellInjector::TypeEnum injectorType;
        WellInjector::ControlModeEnum controlMode;

        bool operator==(const WellInjectionProperties& other) const;
        bool operator!=(const WellInjectionProperties& other) const;

        WellInjectionProperties();
        void handleWELTARG(WellTarget::ControlModeEnum cmode, double newValue, double siFactorG, double siFactorL, double siFactorP);
        void handleWCONINJE(const DeckRecord& record, bool availableForGroupControl, const std::string& well_name, const UnitSystem& unit_system);
        void handleWCONINJH(const DeckRecord& record, bool is_producer, const std::string& well_name, const UnitSystem& unit_system);
        bool hasInjectionControl(WellInjector::ControlModeEnum controlModeArg) const {
            if (injectionControls & controlModeArg)
                return true;
            else
                return false;
        }

        void dropInjectionControl(WellInjector::ControlModeEnum controlModeArg) {
            if ((injectionControls & controlModeArg) != 0)
                injectionControls -= controlModeArg;
        }

        void addInjectionControl(WellInjector::ControlModeEnum controlModeArg) {
            if ((injectionControls & controlModeArg) == 0)
                injectionControls += controlModeArg;
        }

        void resetDefaultHistoricalBHPLimit();

        void setBHPLimit(const double limit);
        InjectionControls controls(const SummaryState& st) const;
    };

    std::ostream& operator<<( std::ostream&, const WellInjectionProperties& );
}

#endif
