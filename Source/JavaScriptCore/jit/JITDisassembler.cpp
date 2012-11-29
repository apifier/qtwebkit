/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JITDisassembler.h"

#if ENABLE(JIT)

#include "CodeBlock.h"
#include "JIT.h"

namespace JSC {

JITDisassembler::JITDisassembler(CodeBlock *codeBlock)
    : m_codeBlock(codeBlock)
    , m_labelForBytecodeIndexInMainPath(codeBlock->instructionCount())
    , m_labelForBytecodeIndexInSlowPath(codeBlock->instructionCount())
{
}

JITDisassembler::~JITDisassembler()
{
}

void JITDisassembler::dump(PrintStream& out, LinkBuffer& linkBuffer)
{
    out.print("Baseline JIT code for CodeBlock ", RawPointer(m_codeBlock), ", instruction count = ", m_codeBlock->instructionCount(), "\n");
    out.print("   Code at [", RawPointer(linkBuffer.debugAddress()), ", ", RawPointer(static_cast<char*>(linkBuffer.debugAddress()) + linkBuffer.debugSize()), "):\n");
    dumpDisassembly(out, linkBuffer, m_startOfCode, m_labelForBytecodeIndexInMainPath[0]);
    
    MacroAssembler::Label firstSlowLabel;
    for (unsigned i = 0; i < m_labelForBytecodeIndexInSlowPath.size(); ++i) {
        if (m_labelForBytecodeIndexInSlowPath[i].isSet()) {
            firstSlowLabel = m_labelForBytecodeIndexInSlowPath[i];
            break;
        }
    }
    dumpForInstructions(out, linkBuffer, "    ", m_labelForBytecodeIndexInMainPath, firstSlowLabel.isSet() ? firstSlowLabel : m_endOfSlowPath);
    out.print("    (End Of Main Path)\n");
    dumpForInstructions(out, linkBuffer, "    (S) ", m_labelForBytecodeIndexInSlowPath, m_endOfSlowPath);
    out.print("    (End Of Slow Path)\n");

    dumpDisassembly(out, linkBuffer, m_endOfSlowPath, m_endOfCode);
}

void JITDisassembler::dump(LinkBuffer& linkBuffer)
{
    dump(WTF::dataFile(), linkBuffer);
}

void JITDisassembler::dumpForInstructions(PrintStream& out, LinkBuffer& linkBuffer, const char* prefix, Vector<MacroAssembler::Label>& labels, MacroAssembler::Label endLabel)
{
    for (unsigned i = 0 ; i < labels.size();) {
        if (!labels[i].isSet()) {
            i++;
            continue;
        }
        out.print(prefix);
        m_codeBlock->dump(i);
        for (unsigned nextIndex = i + 1; ; nextIndex++) {
            if (nextIndex >= labels.size()) {
                dumpDisassembly(out, linkBuffer, labels[i], endLabel);
                return;
            }
            if (labels[nextIndex].isSet()) {
                dumpDisassembly(out, linkBuffer, labels[i], labels[nextIndex]);
                i = nextIndex;
                break;
            }
        }
    }
}

void JITDisassembler::dumpDisassembly(PrintStream& out, LinkBuffer& linkBuffer, MacroAssembler::Label from, MacroAssembler::Label to)
{
    CodeLocationLabel fromLocation = linkBuffer.locationOf(from);
    CodeLocationLabel toLocation = linkBuffer.locationOf(to);
    disassemble(fromLocation, bitwise_cast<uintptr_t>(toLocation.executableAddress()) - bitwise_cast<uintptr_t>(fromLocation.executableAddress()), "        ", out);
}

} // namespace JSC

#endif // ENABLE(JIT)

