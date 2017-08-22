#pragma once

#include "hook_utils.hpp"

template<class FunctionType, class RealFunctionType = FunctionType>
class Hook
{
public:
    Hook(const Hook&) = delete;
    Hook& operator = (const Hook&) = delete;
    explicit Hook(FunctionType oldFunc)
        : mOldPtr(oldFunc)
    {

    }

    explicit Hook(DWORD oldFunc)
        : mOldPtr(reinterpret_cast<FunctionType>(oldFunc))
    {

    }

    void Install(FunctionType newFunc)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)mOldPtr, newFunc);
        if (DetourTransactionCommit() != NO_ERROR)
        {
            Utils::FatalExit("detouring failed");
        }
    }

    RealFunctionType Real() const
    {
#pragma warning(suppress: 4191)
        return reinterpret_cast<RealFunctionType>(mOldPtr);
    }

private:
    FunctionType mOldPtr;
};
