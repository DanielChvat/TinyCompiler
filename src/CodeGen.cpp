#include "CodeGen.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <stdexcept>
#include <vector>

CodeGen::CodeGen(): builder(context) {
    module = std::make_unique<llvm::Module>("top_module", context);
}

void CodeGen::generate(const Program& program) {
    for (const auto& fn : program.functions) {
        declareFunction(*fn);
    }

    for (const auto& fn : program.functions) {
        codegenFunctionBody(*fn);
    }
}

void CodeGen::printIR() {
    module->print(llvm::outs(), nullptr);
}

llvm::Type* CodeGen::llvmType(TypeKind type) {
    switch (type) {
        case TypeKind::Short:
            return llvm::Type::getInt16Ty(context);
        case TypeKind::Int:
            return llvm::Type::getInt32Ty(context);
        case TypeKind::Float:
            return llvm::Type::getFloatTy(context);
        case TypeKind::Double:
            return llvm::Type::getDoubleTy(context);
        case TypeKind::Void:
            return llvm::Type::getVoidTy(context);
    }

    throw std::runtime_error("Unknown type");
}

llvm::Constant* CodeGen::zeroValue(TypeKind type) {
    if (type == TypeKind::Short || type == TypeKind::Int) {
        return llvm::ConstantInt::get(llvmType(type), 0);
    }

    if (type == TypeKind::Float || type == TypeKind::Double) {
        return llvm::ConstantFP::get(llvmType(type), 0.0);
    }

    throw std::runtime_error("Cannot create zero value for void");
}

bool CodeGen::isInteger(TypeKind type) const {
    return type == TypeKind::Short ||
           type == TypeKind::Int;
}

bool CodeGen::isFloating(TypeKind type) const {
    return type == TypeKind::Float ||
           type == TypeKind::Double;
}

TypeKind CodeGen::commonType(TypeKind lhs, TypeKind rhs) const {
    if (lhs == TypeKind::Double || rhs == TypeKind::Double) {
        return TypeKind::Double;
    }

    if (lhs == TypeKind::Float || rhs == TypeKind::Float) {
        return TypeKind::Float;
    }

    if (lhs == TypeKind::Int || rhs == TypeKind::Int) {
        return TypeKind::Int;
    }

    return TypeKind::Short;
}

TypedValue CodeGen::castTo(TypedValue input, TypeKind targetType) {
    if (input.type == targetType) {
        return input;
    }

    if (input.type == TypeKind::Void || targetType == TypeKind::Void) {
        throw std::runtime_error("Cannot cast void value");
    }

    llvm::Type* targetLLVMType = llvmType(targetType);

    bool fromInteger = isInteger(input.type);
    bool toInteger = isInteger(targetType);

    bool fromFloating = isFloating(input.type);
    bool toFloating = isFloating(targetType);

    if (fromInteger && toInteger) {
        unsigned fromBits = input.value->getType()->getIntegerBitWidth();
        unsigned toBits = targetLLVMType->getIntegerBitWidth();

        if (fromBits < toBits) {
            return {
                builder.CreateSExt(input.value, targetLLVMType, "sexttmp"),
                targetType
            };
        }

        if (fromBits > toBits) {
            return {
                builder.CreateTrunc(input.value, targetLLVMType, "trunctmp"),
                targetType
            };
        }

        return input;
    }

    if (fromInteger && toFloating) {
        return {
            builder.CreateSIToFP(input.value, targetLLVMType, "sitofptmp"),
            targetType
        };
    }

    if (fromFloating && toInteger) {
        return {
            builder.CreateFPToSI(input.value, targetLLVMType, "fptositmp"),
            targetType
        };
    }

    if (fromFloating && toFloating) {
        if (input.type == TypeKind::Float && targetType == TypeKind::Double) {
            return {
                builder.CreateFPExt(input.value, targetLLVMType, "fpexttmp"),
                targetType
            };
        }

        if (input.type == TypeKind::Double && targetType == TypeKind::Float) {
            return {
                builder.CreateFPTrunc(input.value, targetLLVMType, "fptrunctmp"),
                targetType
            };
        }

        return input;
    }

    throw std::runtime_error("Unsupported cast");
}

llvm::AllocaInst* CodeGen::createEntryBlockAlloca(
    llvm::Function* function,
    const std::string& name,
    llvm::Type* type
) {
    llvm::IRBuilder<> tmpBuilder(
        &function->getEntryBlock(),
        function->getEntryBlock().begin()
    );

    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Function* CodeGen::declareFunction(const Function& fn) {
    if (module->getFunction(fn.name)) {
        throw std::runtime_error("Function already declared: " + fn.name);
    }

    std::vector<llvm::Type*> paramTypes;

    for (const Param& param : fn.params) {
        paramTypes.push_back(llvmType(param.type));
    }

    llvm::FunctionType* functionType =
        llvm::FunctionType::get(
            llvmType(fn.returnType),
            paramTypes,
            false
        );

    llvm::Function* function =
        llvm::Function::Create(
            functionType,
            llvm::Function::ExternalLinkage,
            fn.name,
            module.get()
        );

    unsigned index = 0;

    for (auto& arg : function->args()) {
        arg.setName(fn.params[index].name);
        ++index;
    }

    functionReturnTypes[fn.name] = fn.returnType;

    return function;
}

void CodeGen::codegenFunctionBody(const Function& fn) {
    llvm::Function* function = module->getFunction(fn.name);

    if (!function) {
        throw std::runtime_error("Function was not declared: " + fn.name);
    }

    llvm::BasicBlock* entry =
        llvm::BasicBlock::Create(context, "entry", function);

    builder.SetInsertPoint(entry);
    namedValues.clear();

    currentReturnType = fn.returnType;

    unsigned index = 0;
    for (auto& arg : function->args()) {
        const Param& param = fn.params[index];

        llvm::AllocaInst* alloca =
            createEntryBlockAlloca(
                function,
                param.name,
                llvmType(param.type)
            );

        builder.CreateStore(&arg, alloca);

        namedValues[param.name] = VariableInfo{
            param.type,
            alloca
        };

        ++index;
    }

    codegenBlock(*fn.body);

    if (!builder.GetInsertBlock()->getTerminator()) {
        if (fn.returnType == TypeKind::Void) {
            builder.CreateRetVoid();
        } else {
            builder.CreateRet(zeroValue(fn.returnType));
        }
    }

    if (llvm::verifyFunction(*function, &llvm::errs())) {
        throw std::runtime_error(
            "Generated invalid LLVM IR for function: " + fn.name
        );
    }
}

void CodeGen::codegenBlock(const BlockStmt& block) {
    for (const auto& stmt : block.statements) {
        if (builder.GetInsertBlock()->getTerminator()) {
            return;
        }
        codegenStmt(*stmt);
    }
}

void CodeGen::codegenStmt(const Stmt& stmt) {
    // Variable declaration:
    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        llvm::Function* function =
            builder.GetInsertBlock()->getParent();

        llvm::AllocaInst* alloca =
            createEntryBlockAlloca(
                function,
                s->name,
                llvmType(s->type)
            );

        TypedValue initValue = codegenExpr(*s->init);
        initValue = castTo(initValue, s->type);
        builder.CreateStore(initValue.value, alloca);
        namedValues[s->name] = VariableInfo{
            s->type,
            alloca
        };
        return;
    }

    // Assignment:
    if (auto* s = dynamic_cast<const AssignStmt*>(&stmt)) {
        auto it = namedValues.find(s->name);

        if (it == namedValues.end()) {
            throw std::runtime_error("Unknown variable: " + s->name);
        }

        TypedValue value = codegenExpr(*s->value);
        value = castTo(value, it->second.type);
        builder.CreateStore(value.value, it->second.alloca);
        return;
    }

    // Return statement:
    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        if (currentReturnType == TypeKind::Void) {
            throw std::runtime_error("Cannot return a value from void function");
        }

        TypedValue value = codegenExpr(*s->value);
        value = castTo(value, currentReturnType);
        builder.CreateRet(value.value);
        return;
    }

    // Expression statement:
    if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        codegenExpr(*s->expr);
        return;
    }

    // If statement:
    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        llvm::Value* cond = truthy(codegenExpr(*s->cond));

        llvm::Function* function =
            builder.GetInsertBlock()->getParent();

        llvm::BasicBlock* thenBB =
            llvm::BasicBlock::Create(context, "then", function);

        llvm::BasicBlock* elseBB =
            llvm::BasicBlock::Create(context, "else");

        llvm::BasicBlock* mergeBB =
            llvm::BasicBlock::Create(context, "if.end");

        builder.CreateCondBr(cond, thenBB, elseBB);

        builder.SetInsertPoint(thenBB);
        codegenBlock(*s->thenBlock);

        bool thenNeedsMerge = !builder.GetInsertBlock()->getTerminator();
        if (thenNeedsMerge) {
            builder.CreateBr(mergeBB);
        }

        function->insert(function->end(), elseBB);
        builder.SetInsertPoint(elseBB);
        if (s->elseBlock) {
            codegenBlock(*s->elseBlock);
        }

        bool elseNeedsMerge = !builder.GetInsertBlock()->getTerminator();
        if (elseNeedsMerge) {
            builder.CreateBr(mergeBB);
        }

        if (thenNeedsMerge || elseNeedsMerge) {
            function->insert(function->end(), mergeBB);
            builder.SetInsertPoint(mergeBB);
        } else {
            delete mergeBB;
        }
        
        return;
    }

    // While loop:
    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        llvm::Function* function =
            builder.GetInsertBlock()->getParent();

        llvm::BasicBlock* condBB =
            llvm::BasicBlock::Create(context, "while.cond", function);

        llvm::BasicBlock* bodyBB =
            llvm::BasicBlock::Create(context, "while.body");

        llvm::BasicBlock* endBB =
            llvm::BasicBlock::Create(context, "while.end");

        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        llvm::Value* cond = truthy(codegenExpr(*s->cond));
        builder.CreateCondBr(cond, bodyBB, endBB);
        function->insert(function->end(), bodyBB);
        builder.SetInsertPoint(bodyBB);
        codegenBlock(*s->body);
        
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(condBB);
        }
        function->insert(function->end(), endBB);
        builder.SetInsertPoint(endBB);
        return;
    }

    throw std::runtime_error("Unknown statement kind");
}

TypedValue CodeGen::codegenExpr(const Expr& expr) {
    // Integer literal:
    if (auto* e = dynamic_cast<const IntegerLiteralExpr*>(&expr)) {
        return {
            llvm::ConstantInt::get(
                llvm::Type::getInt32Ty(context),
                e->value,
                true
            ),
            TypeKind::Int
        };
    }

    // Floating literal:
    if (auto* e = dynamic_cast<const FloatingLiteralExpr*>(&expr)) {
        return {
            llvm::ConstantFP::get(
                llvm::Type::getDoubleTy(context),
                e->value
            ),
            TypeKind::Double
        };
    }

    // Variable reference:
    if (auto* e = dynamic_cast<const VariableExpr*>(&expr)) {
        auto it = namedValues.find(e->name);

        if (it == namedValues.end()) {
            throw std::runtime_error("Unknown variable: " + e->name);
        }

        return {
            builder.CreateLoad(
                llvmType(it->second.type),
                it->second.alloca,
                e->name + ".load"
            ),
            it->second.type
        };
    }

    // Binary expression:
    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        TypedValue lhs = codegenExpr(*e->lhs);
        TypedValue rhs = codegenExpr(*e->rhs);

        TypeKind targetType = commonType(lhs.type, rhs.type);

        lhs = castTo(lhs, targetType);
        rhs = castTo(rhs, targetType);

        if (e->op == "+") {
            if (isFloating(targetType)) {
                return {
                    builder.CreateFAdd(lhs.value, rhs.value, "faddtmp"),
                    targetType
                };
            }

            return {
                builder.CreateAdd(lhs.value, rhs.value, "addtmp"),
                targetType
            };
        }

        if (e->op == "-") {
            if (isFloating(targetType)) {
                return {
                    builder.CreateFSub(lhs.value, rhs.value, "fsubtmp"),
                    targetType
                };
            }

            return {
                builder.CreateSub(lhs.value, rhs.value, "subtmp"),
                targetType
            };
        }

        if (e->op == "*") {
            if (isFloating(targetType)) {
                return {
                    builder.CreateFMul(lhs.value, rhs.value, "fmultmp"),
                    targetType
                };
            }

            return {
                builder.CreateMul(lhs.value, rhs.value, "multmp"),
                targetType
            };
        }

        if (e->op == "/") {
            if (isFloating(targetType)) {
                return {
                    builder.CreateFDiv(lhs.value, rhs.value, "fdivtmp"),
                    targetType
                };
            }

            return {
                builder.CreateSDiv(lhs.value, rhs.value, "divtmp"),
                targetType
            };
        }

        llvm::Value* cmp = nullptr;

        if (isFloating(targetType)) {
            if (e->op == "==") {
                cmp = builder.CreateFCmpOEQ(lhs.value, rhs.value, "fcmptmp");

            } else if (e->op == "!=") {
                cmp = builder.CreateFCmpONE(lhs.value, rhs.value, "fcmptmp");

            } else if (e->op == "<") {
                cmp = builder.CreateFCmpOLT(lhs.value, rhs.value, "fcmptmp");

            } else if (e->op == "<=") {
                cmp = builder.CreateFCmpOLE(lhs.value, rhs.value, "fcmptmp");

            } else if (e->op == ">") {
                cmp = builder.CreateFCmpOGT(lhs.value, rhs.value, "fcmptmp");

            } else if (e->op == ">=") {
                cmp = builder.CreateFCmpOGE(lhs.value, rhs.value, "fcmptmp");
            }

        } else {
            if (e->op == "==") {
                cmp = builder.CreateICmpEQ(lhs.value, rhs.value, "icmptmp");

            } else if (e->op == "!=") {
                cmp = builder.CreateICmpNE(lhs.value, rhs.value, "icmptmp");

            } else if (e->op == "<") {
                cmp = builder.CreateICmpSLT(lhs.value, rhs.value, "icmptmp");

            } else if (e->op == "<=") {
                cmp = builder.CreateICmpSLE(lhs.value, rhs.value, "icmptmp");

            } else if (e->op == ">") {
                cmp = builder.CreateICmpSGT(lhs.value, rhs.value, "icmptmp");

            } else if (e->op == ">=") {
                cmp = builder.CreateICmpSGE(lhs.value, rhs.value, "icmptmp");
            }
        }

        if (!cmp) {
            throw std::runtime_error("Unknown binary operator: " + e->op);
        }

        return {
            builder.CreateZExt(
                cmp,
                llvm::Type::getInt32Ty(context),
                "booltmp"
            ),
            TypeKind::Int
        };
    }

    // Function call:
    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        llvm::Function* callee = module->getFunction(e->caller_name);

        if (!callee) {
            throw std::runtime_error("Unknown function: " + e->caller_name);
        }

        if (callee->arg_size() != e->args.size()) {
            throw std::runtime_error(
                "Wrong number of arguments to function: " + e->caller_name
            );
        }

        std::vector<llvm::Value*> argValues;
        auto argIt = callee->arg_begin();
        for (const auto& argExpr : e->args) {
            TypedValue argValue = codegenExpr(*argExpr);
            llvm::Type* expectedLLVMType = argIt->getType();
            TypeKind expectedType;
            if (expectedLLVMType->isIntegerTy(16)) {
                expectedType = TypeKind::Short;
            } else if (expectedLLVMType->isIntegerTy(32)) {
                expectedType = TypeKind::Int;
            } else if (expectedLLVMType->isFloatTy()) {
                expectedType = TypeKind::Float;
            } else if (expectedLLVMType->isDoubleTy()) {
                expectedType = TypeKind::Double;
            } else {
                throw std::runtime_error("Unsupported function parameter type");
            }

            argValue = castTo(argValue, expectedType);
            argValues.push_back(argValue.value);
            ++argIt;
        }

        TypeKind returnType = functionReturnTypes.at(e->caller_name);

        if (returnType == TypeKind::Void) {
            builder.CreateCall(callee, argValues);

            return {
                llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(context),
                    0
                ),
                TypeKind::Int
            };
        }

        return {
            builder.CreateCall(callee, argValues, "calltmp"),
            returnType
        };
    }

    throw std::runtime_error("Unknown expression kind");
}

llvm::Value* CodeGen::truthy(TypedValue value) {
    if (isFloating(value.type)) {
        return builder.CreateFCmpONE(
            value.value,
            llvm::ConstantFP::get(llvmType(value.type), 0.0),
            "truthy"
        );
    }

    if (isInteger(value.type)) {
        return builder.CreateICmpNE(
            value.value,
            llvm::ConstantInt::get(llvmType(value.type), 0),
            "truthy"
        );
    }

    throw std::runtime_error("Cannot use void value as condition");
}