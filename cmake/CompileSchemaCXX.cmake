# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp -o "${APSU_BUILD_DIR}/common/apsu" "${APSU_SOURCE_DIR}/common/apsu/psi_params.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp -o "${APSU_BUILD_DIR}/common/apsu/network" "${APSU_SOURCE_DIR}/common/apsu/network/ciphertext.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp --include-prefix "apsu/network/" -o "${APSU_BUILD_DIR}/common/apsu/network" "${APSU_SOURCE_DIR}/common/apsu/network/sop.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp -o "${APSU_BUILD_DIR}/common/apsu/network" "${APSU_SOURCE_DIR}/common/apsu/network/sop_header.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp --include-prefix "apsu/network/" -o "${APSU_BUILD_DIR}/common/apsu/network" "${APSU_SOURCE_DIR}/common/apsu/network/sop_response.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp --include-prefix "apsu/network/" -o "${APSU_BUILD_DIR}/common/apsu/network" "${APSU_SOURCE_DIR}/common/apsu/network/result_package.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp --include-prefix "apsu/" -o "${APSU_BUILD_DIR}/sender/apsu" "${APSU_SOURCE_DIR}/sender/apsu/bin_bundle.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()

execute_process(
    COMMAND ${FLATBUFFERS_FLATC_PATH} --cpp --include-prefix "apsu/" -I "${APSU_SOURCE_DIR}/common/apsu" -o "${APSU_BUILD_DIR}/sender/apsu" "${APSU_SOURCE_DIR}/sender/apsu/sender_db.fbs"
    OUTPUT_QUIET
    RESULT_VARIABLE result)
if(result)
    message(FATAL_ERROR "flatc failed (${result})")
endif()
