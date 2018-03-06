#include <iostream>

#include "talvos/Device.h"
#include "talvos/Instruction.h"
#include "talvos/Invocation.h"
#include "talvos/Plugin.h"

using namespace talvos;

class CallbackTest : public Plugin
{
public:
  CallbackTest() { std::cout << "plugin created" << std::endl; }

  ~CallbackTest() { std::cout << "plugin destroyed" << std::endl; }

  void dispatchCommandBegin(const DispatchCommand *Cmd) override
  {
    std::cout << "dispatchcommand begin" << std::endl;
  }

  void dispatchCommandComplete(const DispatchCommand *Cmd) override
  {
    std::cout << "dispatchcommand complete" << std::endl;
  }

  void instructionExecuted(const Invocation *Invoc,
                           const Instruction *Inst) override
  {
    std::cout << Invoc->getGlobalId() << ": ";
    Inst->print(std::cout);
    std::cout << std::endl;
  }

  void invocationBegin(const Invocation *Invoc) override
  {
    std::cout << "invocation begin" << std::endl;
  }

  void invocationComplete(const Invocation *Invoc) override
  {
    std::cout << "invocation complete" << std::endl;
  }

  void workgroupBegin(const Workgroup *Group) override
  {
    std::cout << "workgroup begin" << std::endl;
  }

  void workgroupBarrier(const Workgroup *Group) override
  {
    std::cout << "workgroup barrier" << std::endl;
  }

  void workgroupComplete(const Workgroup *Group) override
  {
    std::cout << "workgroup complete" << std::endl;
  }
};

extern "C"
{
  Plugin *talvosCreatePlugin(const Device *Dev) { return new CallbackTest; }
  void talvosDestroyPlugin(Plugin *P) { delete P; }
}
