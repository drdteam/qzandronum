
#include "r_compiler/llvm_include.h"
#include "ssa_ubyte_ptr.h"
#include "ssa_scope.h"

SSAUBytePtr::SSAUBytePtr()
: v(0)
{
}

SSAUBytePtr::SSAUBytePtr(llvm::Value *v)
: v(v)
{
}

llvm::Type *SSAUBytePtr::llvm_type()
{
	return llvm::Type::getInt8PtrTy(SSAScope::context());
}

SSAUBytePtr SSAUBytePtr::operator[](SSAInt index) const
{
	return SSAUBytePtr::from_llvm(SSAScope::builder().CreateGEP(v, index.v, SSAScope::hint()));
}

SSAUByte SSAUBytePtr::load(bool constantScopeDomain) const
{
	auto loadInst = SSAScope::builder().CreateLoad(v, false, SSAScope::hint());
	if (constantScopeDomain)
		loadInst->setMetadata(llvm::LLVMContext::MD_alias_scope, SSAScope::constant_scope_list());
	return SSAUByte::from_llvm(loadInst);
}

SSAVec4i SSAUBytePtr::load_vec4ub(bool constantScopeDomain) const
{
	auto loadInst = SSAScope::builder().CreateLoad(SSAScope::builder().CreateBitCast(v, llvm::Type::getInt32PtrTy(SSAScope::context()), SSAScope::hint()), false, SSAScope::hint());
	if (constantScopeDomain)
		loadInst->setMetadata(llvm::LLVMContext::MD_alias_scope, SSAScope::constant_scope_list());
	SSAInt i32 = SSAInt::from_llvm(loadInst);
	return SSAVec4i::unpack(i32);
}

SSAVec16ub SSAUBytePtr::load_vec16ub(bool constantScopeDomain) const
{
	llvm::PointerType *m16xint8typeptr = llvm::VectorType::get(llvm::Type::getInt8Ty(SSAScope::context()), 16)->getPointerTo();
	auto loadInst = SSAScope::builder().CreateAlignedLoad(SSAScope::builder().CreateBitCast(v, m16xint8typeptr, SSAScope::hint()), 16, false, SSAScope::hint());
	if (constantScopeDomain)
		loadInst->setMetadata(llvm::LLVMContext::MD_alias_scope, SSAScope::constant_scope_list());
	return SSAVec16ub::from_llvm(loadInst);
}

SSAVec16ub SSAUBytePtr::load_unaligned_vec16ub(bool constantScopeDomain) const
{
	llvm::PointerType *m16xint8typeptr = llvm::VectorType::get(llvm::Type::getInt8Ty(SSAScope::context()), 16)->getPointerTo();
	auto loadInst = SSAScope::builder().CreateAlignedLoad(SSAScope::builder().CreateBitCast(v, m16xint8typeptr, SSAScope::hint()), 1, false, SSAScope::hint());
	if (constantScopeDomain)
		loadInst->setMetadata(llvm::LLVMContext::MD_alias_scope, SSAScope::constant_scope_list());
	return SSAVec16ub::from_llvm(loadInst);
}

void SSAUBytePtr::store(const SSAUByte &new_value)
{
	auto inst = SSAScope::builder().CreateStore(new_value.v, v, false);
	inst->setMetadata(llvm::LLVMContext::MD_noalias, SSAScope::constant_scope_list());
}

void SSAUBytePtr::store_vec4ub(const SSAVec4i &new_value)
{
	// Store using saturate:
	SSAVec8s v8s(new_value, new_value);
	SSAVec16ub v16ub(v8s, v8s);
	
	llvm::Type *m16xint8type = llvm::VectorType::get(llvm::Type::getInt8Ty(SSAScope::context()), 16);
	llvm::PointerType *m4xint8typeptr = llvm::VectorType::get(llvm::Type::getInt8Ty(SSAScope::context()), 4)->getPointerTo();
	std::vector<llvm::Constant*> constants;
	constants.push_back(llvm::ConstantInt::get(SSAScope::context(), llvm::APInt(32, 0)));
	constants.push_back(llvm::ConstantInt::get(SSAScope::context(), llvm::APInt(32, 1)));
	constants.push_back(llvm::ConstantInt::get(SSAScope::context(), llvm::APInt(32, 2)));
	constants.push_back(llvm::ConstantInt::get(SSAScope::context(), llvm::APInt(32, 3)));
	llvm::Value *mask = llvm::ConstantVector::get(constants);
	llvm::Value *val_vector = SSAScope::builder().CreateShuffleVector(v16ub.v, llvm::UndefValue::get(m16xint8type), mask, SSAScope::hint());
	llvm::StoreInst *inst = SSAScope::builder().CreateStore(val_vector, SSAScope::builder().CreateBitCast(v, m4xint8typeptr, SSAScope::hint()), false);
	inst->setMetadata(llvm::LLVMContext::MD_noalias, SSAScope::constant_scope_list());
}

void SSAUBytePtr::store_vec16ub(const SSAVec16ub &new_value)
{
	llvm::PointerType *m16xint8typeptr = llvm::VectorType::get(llvm::Type::getInt8Ty(SSAScope::context()), 16)->getPointerTo();
	llvm::StoreInst *inst = SSAScope::builder().CreateAlignedStore(new_value.v, SSAScope::builder().CreateBitCast(v, m16xint8typeptr, SSAScope::hint()), 16);
	inst->setMetadata(llvm::LLVMContext::MD_noalias, SSAScope::constant_scope_list());

	// The following generates _mm_stream_si128, maybe!
	// llvm::MDNode *node = llvm::MDNode::get(SSAScope::context(), SSAScope::builder().getInt32(1));
	// inst->setMetadata(SSAScope::module()->getMDKindID("nontemporal"), node);
}

void SSAUBytePtr::store_unaligned_vec16ub(const SSAVec16ub &new_value)
{
	llvm::PointerType *m16xint8typeptr = llvm::VectorType::get(llvm::Type::getInt8Ty(SSAScope::context()), 16)->getPointerTo();
	llvm::StoreInst *inst = SSAScope::builder().CreateAlignedStore(new_value.v, SSAScope::builder().CreateBitCast(v, m16xint8typeptr, SSAScope::hint()), 1);
	inst->setMetadata(llvm::LLVMContext::MD_noalias, SSAScope::constant_scope_list());
}
