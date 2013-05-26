#include "Processor.hpp"

#include <thread>

std::unique_ptr<ProcessorState>
Processor::run() {
    setActive();

    while ( isActive() ) {
        _proc->executeNext();

        if ( auto ins = dynamic_cast<const instruction::Unary*>( _proc->lastInstruction() ) ) {
            executeSpecial( ins );
        }

        std::this_thread::sleep_for( _tickDuration * _proc->clock() );
        _proc->clearClock();
    }

    return std::move( _proc );
}

void Processor::executeSpecial( const instruction::Unary* ins ) {
    Word index;

    if ( ins->opcode == SpecialOpcode::Hwi ) {
        Word index = ins->address->load( *_proc );
        auto inter = _computer.interruptByIndex( index );

        // Trigger the interrupt, and wait for the device to respond.
        inter->trigger( std::move( _proc ) );
        _proc = inter->waitForResponse();
    } else if ( ins->opcode == SpecialOpcode::Hwn ) {
        ins->address->store( *_proc, _computer.numDevices() );
    }
}
