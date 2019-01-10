//===- GenRuntimeInfoPass.h -----------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef ONNC_TARGET_TG_GEN_RUNTIME_INFO_PASS_H
#define ONNC_TARGET_TG_GEN_RUNTIME_INFO_PASS_H
#include <onnc/Core/DefaultModulePass.h>
#include <onnc/Support/Path.h>
#include "BM188xBackend.h"

namespace onnc {
namespace BM188X {

class GenRuntimeInfoPass : public DefaultModulePass<GenRuntimeInfoPass>
{
public:
  GenRuntimeInfoPass(BM1880Backend* pBackend, const Path &pOutFile);

  Pass::ReturnType runOnModule(Module &pModule) override;

private:
  struct LayerNames {
    std::string onnc;
    std::string onnx;
  };

private:
  BM1880Backend *backend() { return m_pBackend; }

  const BM1880Backend *backend() const { return m_pBackend; }

  float getThreshold(const std::string &pName);

  static std::string
  FindOnncLayerName(const xGraph& pG, const xValue &pValue);

  static void
  GetDefaultLayerNames(LayerNames& pNames, const xGraph& pG);

  void GenOutputLayer(json::Object& pOutput, const LayerNames& pNames,
                      const xGraph& pG);

  void GenFallbackPlan(json::Object& pOutput, const LayerNames& pNames,
                       const xGraph& pG);

  void GenMemoryLayout(json::Object& pOutput, const ComputeGraph& pG);

  void GenRest(json::Object& pOutput, const xGraph& pG);

private:
  BM1880Backend *m_pBackend;
  Path m_OutFile;
};

//===----------------------------------------------------------------------===//
// Factory method
//===----------------------------------------------------------------------===//
ModulePass*
CreateGenRuntimeInfoPass(BM1880Backend* pBackend, const Path& pOutFile);

} // namespace BM188X
} // namespace onnc

#endif
