// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

// STD
#include <memory>

// APSU
#include "apsu/psi_params.h"
#include "sender/clp.h"

std::unique_ptr<apsu::PSIParams> build_psi_params(const CLP &cmd);
