//===- SetMemOperand.h ----------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef ONNC_CODEGEN_SET_MEM_OPERAND_H
#define ONNC_CODEGEN_SET_MEM_OPERAND_H
#include <onnc/Core/CustomPass.h>

namespace onnc {

/** \class SetMemOperand
 *  \brief Setup start and length for ComputeMemOperand according to memory
 *         allocation result
 */
class SetMemOperand : public CustomPass<SetMemOperand>
{
public:
  SetMemOperand() = default;

  StringRef getPassName() const override { return "SetMemOperand"; }

  Pass::ReturnType runOnModule(Module &pModule) override;

  void getAnalysisUsage(AnalysisUsage& pUsage) const override;

private:
};

ModulePass* CreateSetMemOperandPass();

} // namespace onnc

#endif
