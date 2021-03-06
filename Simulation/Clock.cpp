// Simulation/Clock.cpp
//
// Copyright 2013 Jesse Haber-Kucharsky
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Clock.hpp"

namespace nebula {

std::unique_ptr<ClockState>
Clock::run() {
    setActive();

    LOG( CLOCK, info ) << "Simulation is active.";

    while ( isActive() ) {
        if ( ! _state.isOn ) {
            // Do nothing until the clock is triggered by the processor.
            LOG( CLOCK, info ) << "Off. Waiting for interrupt.";

            _procInt->waitForTriggerOrDeath( *this );
        }

        auto now = std::chrono::system_clock::now();

        if ( _procInt->isActive() ) {
            LOG( CLOCK, info ) << "Got interrupt.";

            auto proc = _procInt->state();
            auto a = proc->read( Register::A );

            switch ( a ) {
            case 0:
                handleInterrupt( ClockOperation::SetDivider, proc );
                break;
            case 1:
                handleInterrupt( ClockOperation::StoreElapsed, proc );
                break;
            case 2:
                handleInterrupt( ClockOperation::EnableInterrupts, proc );
                break;
            }
            
            _procInt->respond();

            LOG( CLOCK, info ) << "Handled interrupt.";
        }

        std::this_thread::sleep_until( now + (sim::CLOCK_BASE_PERIOD * _state.divider) );
        _state.elapsed += 1;

        if ( _state.interruptsEnabled ) {
            _computer.queue().push( _state.message );
        }
    }

    LOG( CLOCK, info ) << "Shutting down.";
    return make_unique<ClockState>( _state );
}

void Clock::handleInterrupt( ClockOperation op, ProcessorState* proc ) {
    Word b;

    switch ( op ) {
    case ClockOperation::SetDivider:
        LOG( CLOCK, info ) << "'SetDivider'";

        b = proc->read( Register::B );

        if ( b != 0 ) {
            LOG( CLOCK, info ) << "Turning on with divider " << b << ".";

            _state.isOn = true;
            _state.divider = b;
            _state.elapsed = 0;
        } else {
            LOG( CLOCK, warning ) << "Turning off.";
            _state.isOn = false;
        }

        break;
    case ClockOperation::StoreElapsed:
        LOG( CLOCK, info ) << "'StoreElapsed'";

        proc->write( Register::C, _state.elapsed );

        break;
    case ClockOperation::EnableInterrupts:
        LOG( CLOCK, info ) << "'EnableInterrupts'";

        b = proc->read( Register::B );

        if ( b != 0 ) {
            _state.interruptsEnabled = true;
            _state.message = b;
        } else {
            LOG( CLOCK, warning ) << "Turning interrupts off.";
            _state.interruptsEnabled = false;
        }

        break;
    }
}

}
