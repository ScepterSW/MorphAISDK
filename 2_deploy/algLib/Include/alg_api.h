#pragma once

#include "alg_define.h"

/**
 * @brief        Initialize the algorithm library. This function must be invoked before any other APIs.
 * @param[in]    p_init_param     The pointer of the initialization parameter
 * @return       ::ALGO_RET_OK    If the function succeeded, or one of the error values defined by ::ALGO_RET_E.
 */
ALG_C_API_EXPORT ALGO_RET_E algo_initialize(const ALGO_INIT_PARAM_T *p_init_param);

/**
 * @brief        Start the processing thread of the algorithm
 * @return       ::ALGO_RET_OK    If the function succeeded, or one of the error values defined by ::ALGO_RET_E.
 */
ALG_C_API_EXPORT ALGO_RET_E algo_ctrl_start(void);

/**
 * @brief        Stop the processing thread of the algorithm
 * @return       ::ALGO_RET_OK    If the function succeeded, or one of the error values defined by ::ALGO_RET_E.
 */
ALG_C_API_EXPORT ALGO_RET_E algo_ctrl_stop(void);

/**
 * @brief        Push frame data to the algorithm
 * @param[in]    p_joint_frame    The pointer of the input frame
 * @return       ::ALGO_RET_OK    If the function succeeded, or one of the error values defined by ::ALGO_RET_E.
 */
ALG_C_API_EXPORT ALGO_RET_E algo_push_frame(const CAM_JOINT_FRAME_T *p_joint_frame);

/**
 * @brief        Set the parameters of the algorithm
 * @param[in]    param_id         The id of parameter
 * @param[in]    p_in_param       The pointer of the parameter buffer
 * @param[in]    param_len        The length of the parameter buffer
 * @return       ::ALGO_RET_OK    If the function succeeded, or one of the error values defined by ::ALGO_RET_E.
 */
ALG_C_API_EXPORT ALGO_RET_E algo_set_param(uint32_t param_id, const char *p_in_param, uint16_t param_len);

/**
 * @brief        Get the parameters of the algorithm
 * @param[in]    param_id         The id of parameter
 * @param[out]   p_out_param      The pointer of the parameter buffer
 * @param[out]   p_param_len      The length of the parameter buffer
 * @return       ::ALGO_RET_OK    If the function succeeded, or one of the error values defined by ::ALGO_RET_E.
 */
ALG_C_API_EXPORT ALGO_RET_E algo_get_param(uint32_t param_id, const char **p_out_param, uint16_t *p_param_len);

/**
 * @brief        Releases the resources used by the algorithm library. After invoking this function, no other APIs should be invoked except algo_initialize..
 * @return       ::ALGO_RET_OK    If the function succeeded, or one of the error values defined by ::ALGO_RET_E.
 */
ALG_C_API_EXPORT ALGO_RET_E algo_release(void);