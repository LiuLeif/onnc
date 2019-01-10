//===- PassManagerTest.h --------------------------------------------------===//
//
//                             The ONNC Project
//
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <skypat/skypat.h>
#include <onnc/Core/Pass.h>
#include <onnc/Core/DefaultModulePass.h>
#include <onnc/Core/PassRegistry.h>
#include <onnc/Core/PassSupport.h>
#include <onnc/Core/AnalysisUsage.h>
#include <onnc/Core/PassManager.h>
#include <onnc/IR/Module.h>

using namespace skypat;
using namespace onnc;

namespace {

/** \class A
 */
class A : public DefaultModulePass<A>
{
public:
  A() = default;

  ReturnType runOnModule(Module &pModule) { return kModuleChanged; }

  StringRef getPassName() const { return "A"; }
};

/** \class B
 */
class B : public DefaultModulePass<B>
{

public:
  B() = default;

  ReturnType runOnModule(Module &pModule) { return kModuleChanged; }

  void getAnalysisUsage(AnalysisUsage& pUsage) const {
    pUsage.addRequired<A>();
  }

  StringRef getPassName() const { return "B"; }
};

/** \class C
 */
class C : public DefaultModulePass<C>
{
public:
  C() : data(0x12) { }
  ReturnType runOnModule(Module &pModule) { return kModuleChanged; }

  void getAnalysisUsage(AnalysisUsage& pUsage) const {
    pUsage.addRequired<A>();
    pUsage.addRequired<B>();
  }

  StringRef getPassName() const { return "C"; }

  int data;
};

} // anonymous namespace

INITIALIZE_PASS(A, "A")
INITIALIZE_PASS(B, "B")
INITIALIZE_PASS(C, "C")

//===----------------------------------------------------------------------===//
// PassManagerTest
//===----------------------------------------------------------------------===//
SKYPAT_F(PassManagerTest, add_passes)
{
  PassRegistry registry;
  EXPECT_EQ(registry.numOfPasses(), 0);
  EXPECT_TRUE(registry.isEmpty());

  InitializeAPass(registry);
  InitializeBPass(registry);
  InitializeCPass(registry);

  EXPECT_EQ(registry.numOfPasses(), 3);
  EXPECT_FALSE(registry.isEmpty());
}

SKYPAT_F(PassManagerTest, add_duplicated_passes)
{
  PassRegistry registry;
  EXPECT_EQ(registry.numOfPasses(), 0);
  EXPECT_TRUE(registry.isEmpty());

  InitializeAPass(registry);
  InitializeAPass(registry);
  InitializeAPass(registry);

  EXPECT_EQ(registry.numOfPasses(), 1);
  EXPECT_FALSE(registry.isEmpty());
}

SKYPAT_F(PassManagerTest, delete_passes)
{
  PassRegistry registry;
  InitializeAPass(registry);
  InitializeBPass(registry);
  InitializeCPass(registry);

  ASSERT_EQ(registry.numOfPasses(), 3);
  ASSERT_FALSE(registry.isEmpty());

  registry.clear();

  EXPECT_EQ(registry.numOfPasses(), 0);
  EXPECT_TRUE(registry.isEmpty());
}

SKYPAT_F(PassManagerTest, query_passes)
{
  PassRegistry registry;
  InitializeAPass(registry);
  InitializeBPass(registry);
  InitializeCPass(registry);

  ASSERT_EQ(registry.numOfPasses(), 3);
  ASSERT_FALSE(registry.isEmpty());

  const PassInfo* info = registry.getPassInfo(C::id());
  EXPECT_TRUE(info->getPassName().equals("C"));
  Pass* pass = info->makePass();
  EXPECT_EQ(dynamic_cast<C*>(pass)->data, 0x12);

  PassManager pm(registry);
  pm.add(pass);
  EXPECT_EQ(pm.size(), 3);

  pm.add(pass);
  // no additional passes
  EXPECT_EQ(pm.size(), 3);

  Module module;
  EXPECT_TRUE(pm.run(module));
}

SKYPAT_F(PassManagerTest, add_dependent_passes)
{
  PassRegistry registry;
  InitializeAPass(registry);

  PassManager pm(registry);
  pm.add(new B());
  EXPECT_EQ(pm.size(), 2);
  pm.add(new A());
  EXPECT_EQ(pm.size(), 2);
  pm.add(new C());
  EXPECT_EQ(pm.size(), 3);
}

class X : public DefaultModulePass<X>
{
public:
  X() = default;

  ReturnType runOnModule(Module &pModule) override { return kModuleChanged; }

  StringRef getPassName() const override { return "X"; }
};

INITIALIZE_PASS(X, "X")

class Y : public DefaultModulePass<Y>
{
public:
  Y() = default;

  ReturnType runOnModule(Module &pModule) override { return kModuleNoChanged; }

  StringRef getPassName() const override { return "Y"; }

  void getAnalysisUsage(AnalysisUsage& pUsage) const override {
    pUsage.addRequired<X>();
  }
};

INITIALIZE_PASS(Y, "Y")

class Z : public DefaultModulePass<Z>
{
public:
  Z() = default;

  ReturnType runOnModule(Module &pModule) override {
    static int c = 0;
    ++c;
    if (0 == c % 3)
      return kModuleChanged;

    // retry till c is 3
    return (kPassRetry | kModuleChanged);
  }

  void getAnalysisUsage(AnalysisUsage& pUsage) const override {
    pUsage.addRequired<Y>();
  }

  StringRef getPassName() const override { return "Z"; }
};

INITIALIZE_PASS(Z, "Z")

SKYPAT_F(PassManagerTest, run_test_1)
{
  PassRegistry registry;

  InitializeXPass(registry);
  InitializeYPass(registry);
  InitializeZPass(registry);
  
  ASSERT_EQ(registry.numOfPasses(), 3);
  ASSERT_FALSE(registry.isEmpty());

  PassManager::State state;
  PassManager pm(registry);
  pm.add(new Z(), state);
  pm.add(new Y(), state);
  pm.add(new Z(), state);

  ASSERT_EQ(state.execution.size(), 3); // ZYZ
  ASSERT_FALSE(state.changed);

  Module module;

  std::string process;
  // run Z(1): retry
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 5); // XYZYZ
  ASSERT_FALSE(state.changed);

  // run X: changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 4); // YZYZ
  ASSERT_TRUE(state.changed);

  // run Y: no changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 3); // ZYZ
  ASSERT_TRUE(state.changed);

  // run Z(2): retry
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 5); // XYZYZ
  ASSERT_FALSE(state.changed);

  // run X: changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 4); // YZYZ
  ASSERT_TRUE(state.changed);

  // run Y: no changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 3); // ZYZ
  ASSERT_TRUE(state.changed);

  // run Z(3): changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 2); // YZ
  ASSERT_TRUE(state.changed);

  // run Y: no changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 1); // Z
  ASSERT_TRUE(state.changed);

  // run Z(4): changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 3); // XYZ
  ASSERT_FALSE(state.changed);

  // run X: changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 2); // YZ
  ASSERT_TRUE(state.changed);

  // run Y: no changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 1); // Z
  ASSERT_TRUE(state.changed);

  // run Z(5): retry
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 3); // XYZ
  ASSERT_FALSE(state.changed);

  // run X: changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 2); // YZ
  ASSERT_TRUE(state.changed);

  // run Y: no changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 1); // Z
  ASSERT_TRUE(state.changed);

  // run Z(6): changed
  ASSERT_TRUE(pm.step(module, state));
  process += state.pass->getPassName();
  ASSERT_EQ(state.execution.size(), 0);
  ASSERT_TRUE(state.changed);

  errs() << process << std::endl;
  ASSERT_TRUE(process == "ZXYZXYZYZXYZXYZ");
  ASSERT_TRUE(pm.run(module, state));
}
