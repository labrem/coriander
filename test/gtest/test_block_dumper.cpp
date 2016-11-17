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

#include "basicblockdumper.h"

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

namespace test_block_dumper {

LLVMContext context;
unique_ptr<Module>M;

string ll_path = "../test/gtest/test_block_dumper.ll";  // this is a bit hacky, but fine-ish for now

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

TEST(test_block_dumper, basic) {
    Function *F = getFunction("main");
    F->dump();
    BasicBlock *block = &*F->begin();
    GlobalNames globalNames;
    LocalNames localNames;
    for(auto it=F->arg_begin(); it != F->arg_end(); it++) {
        Argument *arg = &*it;
        localNames.getOrCreateName(arg, arg->getName().str());
    }
    TypeDumper typeDumper(&globalNames);
    FunctionNamesMap functionNamesMap;
    BasicBlockDumper blockDumper(block, &globalNames, &localNames, &typeDumper, &functionNamesMap);
    ostringstream oss;
    std::set< llvm::Function *> dumpedFunctions;
    blockDumper.runGeneration(dumpedFunctions);
    blockDumper.toCl(oss);
    string cl = oss.str();
    cout << cl << endl;
    string expectedBlockCl = R"(    v2 = (5 + 3) + 7;
    v3 = 8.0f + 3.0f;
    v4 = v3 + 9.0f;
    v7 = v6[0];
    v8[0] = v4 / v3;
    v26 = v25[0];
    v26.f0 = 4;
    v28 = v26;
    v28.f1 = 1.5f;
    v29 = v28;
    v8[0] = v3;
    v8[0] = v4;
    v34[0] = v7;
    v34[0] = v7;
    v6[0] = (int)v4;
    v6[0] = (int)v4;
    v8[0] = (float)v2;
    v8[0] = (float)v2;
    structs[0] = v29;
    v6[0] = (v2 < 4) ? 21 : 44;
)";
    ASSERT_EQ(expectedBlockCl, cl);

    cout << "alloca declrations:" << endl;
    cout << blockDumper.getAllocaDeclarations("    ") << endl;
    string expectedAllocaDeclarations = R"(    int v6[1];
    float v8[1];
    struct mystruct v25[1];
    long v34[1];
)";
    ASSERT_EQ(expectedAllocaDeclarations, blockDumper.getAllocaDeclarations("    "));

    cout << "variable declarations:" << endl;
    cout << blockDumper.writeDeclarations("    ") << endl;
    string expectedDeclarations = R"(    int v2;
    float v3;
    float v4;
    int v7;
    struct mystruct v26;
    struct mystruct v28;
    struct mystruct v29;
)";
    ASSERT_EQ(expectedDeclarations, blockDumper.writeDeclarations("    "));
}

TEST(test_block_dumper, basic2) {
    Function *F = getFunction("someKernel");
    F->dump();
    BasicBlock *block = &*F->begin();
    GlobalNames globalNames;
    LocalNames localNames;
    for(auto it=F->arg_begin(); it != F->arg_end(); it++) {
        Argument *arg = &*it;
        string name = arg->getName().str();
        Value *value = arg;
        localNames.getOrCreateName(value, name);
    }
    cout << localNames.dumpNames();
    TypeDumper typeDumper(&globalNames);
    FunctionNamesMap functionNamesMap;
    BasicBlockDumper blockDumper(block, &globalNames, &localNames, &typeDumper, &functionNamesMap);
    ostringstream oss;
    std::set< llvm::Function *> dumpedFunctions;
    blockDumper.runGeneration(dumpedFunctions);
    blockDumper.toCl(oss);
    string cl = oss.str();
    cout << "cl:\n" << cl << endl;
    cout << "allocas: \n" << blockDumper.getAllocaDeclarations("    ") << endl;

    for(auto it=blockDumper.neededFunctions.begin(); it != blockDumper.neededFunctions.end(); it++) {
        cout << "called function " << (*it)->getName().str() << endl;
    }
    ASSERT_EQ(1u, blockDumper.neededFunctions.size());
    ASSERT_EQ(getFunction("someFunc"), *blockDumper.neededFunctions.begin());
}

TEST(test_block_dumper, usesShared) {
    Function *F = getFunction("usesShared");
    F->dump();
    BasicBlock *block = &*F->begin();
    GlobalNames globalNames;
    LocalNames localNames;
    for(auto it=F->arg_begin(); it != F->arg_end(); it++) {
        Argument *arg = &*it;
        string name = arg->getName().str();
        Value *value = arg;
        localNames.getOrCreateName(value, name);
    }
    cout << localNames.dumpNames();
    TypeDumper typeDumper(&globalNames);
    FunctionNamesMap functionNamesMap;
    BasicBlockDumper blockDumper(block, &globalNames, &localNames, &typeDumper, &functionNamesMap);
    ostringstream oss;
    std::set< llvm::Function *> dumpedFunctions;
    blockDumper.runGeneration(dumpedFunctions);
    blockDumper.toCl(oss);
    string cl = oss.str();
    cout << "cl:\n" << cl << endl;
    cout << "allocas: \n" << blockDumper.getAllocaDeclarations("    ") << endl;

    cout << "num shared variables to declare: " << blockDumper.sharedVariablesToDeclare.size() << endl;
    ASSERT_EQ(1u, blockDumper.sharedVariablesToDeclare.size());
    for(auto it=blockDumper.sharedVariablesToDeclare.begin(); it !=blockDumper.sharedVariablesToDeclare.end(); it++) {
        Value *value = *it;
        cout << "shared:" << endl;
        value->dump();
        cout << endl;
    }
    Value *shared = *blockDumper.sharedVariablesToDeclare.begin();
    shared->dump();
}

TEST(test_block_dumper, usesPointerFunction) {
    Function *F = getFunction("usesPointerFunction");
    F->dump();
    BasicBlock *block = &*F->begin();
    GlobalNames globalNames;
    LocalNames localNames;
    for(auto it=F->arg_begin(); it != F->arg_end(); it++) {
        Argument *arg = &*it;
        string name = arg->getName().str();
        Value *value = arg;
        localNames.getOrCreateName(value, name);
    }
    cout << localNames.dumpNames();
    TypeDumper typeDumper(&globalNames);
    FunctionNamesMap functionNamesMap;
    BasicBlockDumper blockDumper(block, &globalNames, &localNames, &typeDumper, &functionNamesMap);
    ostringstream oss;
    std::set< llvm::Function *> dumpedFunctions;
    bool generation_result = blockDumper.runGeneration(dumpedFunctions);
    cout << "generation_result " << generation_result << endl;
    ASSERT_FALSE(generation_result);
    blockDumper.toCl(oss);
    string cl = oss.str();
    cout << "cl:\n" << cl << endl;
    // cout << "allocas: \n" << blockDumper.getAllocaDeclarations("    ") << endl;

    // Function *F2 = getFunction("returnsPointer");

    // BasicBlockDumper blockDumper2(block, &globalNames, &localNames, &typeDumper, &functionNamesMap);
    // ostringstream oss;
    // bool generation_result = blockDumper2.runGeneration();
    // cout << "generation_result " << generation_result << endl;
    // ASSERT_FALSE(generation_result);
    // blockDumper.toCl(oss);
    // string cl = oss.str();
    // cout << "cl:\n" << cl << endl;

    // cout << "num shared variables to declare: " << blockDumper.sharedVariablesToDeclare.size() << endl;
    // ASSERT_EQ(1, blockDumper.sharedVariablesToDeclare.size());
    // for(auto it=blockDumper.sharedVariablesToDeclare.begin(); it !=blockDumper.sharedVariablesToDeclare.end(); it++) {
    //     Value *value = *it;
    //     cout << "shared:" << endl;
    //     value->dump();
    //     cout << endl;
    // }
    // Value *shared = *blockDumper.sharedVariablesToDeclare.begin();
    // shared->dump();
}

} // test_block_dumper