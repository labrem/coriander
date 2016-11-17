// Copyright Hugh Perkins 2016

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "kernel_dumper.h"

#include "type_dumper.h"
#include "GlobalNames.h"
#include "LocalNames.h"

#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>

#include "gtest/gtest.h"

using namespace std;
using namespace cocl;
using namespace llvm;

namespace test_kernel_dumper {

LLVMContext context;
unique_ptr<Module>M;

string ll_path = "../test/gtest/test_kernel_dumper.ll";  // this is a bit hacky, but fine-ish for now

Module *getM() {
    if(M == nullptr) {
        SMDiagnostic smDiagnostic;
        M = parseIRFile(StringRef(ll_path), smDiagnostic, context);
        if(!M) {
            smDiagnostic.print("irtopencl", errs());
            // return "";
            throw runtime_error("failed to parse IR");
            }
    }
    return M.get();
}

Function *getFunction(string name) {
    // Module *M = getM();
    getM();
    Function *F = M->getFunction(StringRef(name));
    if(F == 0) {
        throw runtime_error("Function " + name + " not found");
    }
    return F;
}

TEST(test_kernel_dumper, basic) {
    Module *M = getM();

    // GlobalNames globalNames;
    // LocalNames localNames;
    // TypeDumper typeDumper(&globalNames);
    // FunctionNamesMap functionNamesMap;
    // FunctionDumper functionDumper(F, true, &globalNames, &typeDumper, &functionNamesMap);

    KernelDumper kernelDumper(M, "someKernel");
    string cl = kernelDumper.toCl();
    cout << "kernel cl:\n" << cl << endl;
}

TEST(test_kernel_dumper, kernelBranches) {
    Module *M = getM();

    // GlobalNames globalNames;
    // LocalNames localNames;
    // TypeDumper typeDumper(&globalNames);
    // FunctionNamesMap functionNamesMap;
    // FunctionDumper functionDumper(F, true, &globalNames, &typeDumper, &functionNamesMap);

    KernelDumper kernelDumper(M, "kernelBranches");
    string cl = kernelDumper.toCl();
    cout << "kernel cl:\n" << cl << endl;
}

TEST(test_kernel_dumper, usesPointerFunction) {
    Module *M = getM();

    // GlobalNames globalNames;
    // LocalNames localNames;
    // TypeDumper typeDumper(&globalNames);
    // FunctionNamesMap functionNamesMap;
    // FunctionDumper functionDumper(F, true, &globalNames, &typeDumper, &functionNamesMap);

    KernelDumper kernelDumper(M, "usesPointerFunction");
    string cl = kernelDumper.toCl();
    cout << "kernel cl:\n" << cl << endl;
    ASSERT_TRUE(cl.find("returnsPointer") != string::npos);
}

} // test_block_dumper