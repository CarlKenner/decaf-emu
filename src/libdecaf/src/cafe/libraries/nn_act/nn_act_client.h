#pragma once
#include "nn/nn_result.h"

#include "cafe/nn/cafe_nn_ipc_client.h"
#include "cafe/nn/cafe_nn_ipc_bufferallocator.h"

namespace cafe::nn_act
{

nn::Result
Initialize();

nn::Result
Finalize();

bool
IsParentalControlCheckEnabled();

void
EnableParentalControlCheck(bool enable);

namespace internal
{

virt_ptr<nn::ipc::Client>
getClient();

virt_ptr<nn::ipc::BufferAllocator>
getAllocator();

} // namespace internal

} // namespace cafe::nn_act
