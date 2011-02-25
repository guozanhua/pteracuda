// -------------------------------------------------------------------
//
// pteracuda: An Erlang framework for performing CUDA-enabled operations
//
// Copyright (c) 2011 Hypothetical Labs, Inc. All Rights Reserved.
//
// This file is provided to you under the Apache License,
// Version 2.0 (the "License"); you may not use this file
// except in compliance with the License.  You may obtain
// a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// -------------------------------------------------------------------
#include <string.h>

#include "erl_nif.h"
#include "pteracuda_common.h"
#include "pcuda_worker.h"

extern "C" {
    static int pteracuda_on_load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info);

    ERL_NIF_TERM pteracuda_nifs_new_worker(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
    ERL_NIF_TERM pteracuda_nifs_destroy_worker(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);

    static ErlNifFunc pteracuda_nif_funcs[] = {
        {"new_worker", 0, pteracuda_nifs_new_worker},
        {"destroy_worker", 1, pteracuda_nifs_destroy_worker}
    };
}

ERL_NIF_INIT(pteracuda_nifs, pteracuda_nif_funcs, &pteracuda_on_load, NULL, NULL, NULL);

static int pteracuda_on_load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info) {
    ATOM_OK = enif_make_atom(env, "ok");
    ATOM_ERROR = enif_make_atom(env, "error");
    pteracuda_worker_resource = enif_open_resource_type(env, NULL, "pteracuda_worker_resource",
                                                        NULL, ERL_NIF_RT_CREATE, 0);
    /* Pre-alloate OOM error in case we run out of memory later */
    OOM_ERROR = enif_make_tuple2(env, ATOM_ERROR, enif_make_atom(env, "out_of_memory"));
    return 0;
}

ERL_NIF_TERM pteracuda_nifs_new_worker(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    ERL_NIF_TERM retval = ATOM_ERROR;
    PcudaWorker *worker = new PcudaWorker();
    pcuda_worker_handle *handle = (pcuda_worker_handle *) enif_alloc_resource(pteracuda_worker_resource,
                                                                              sizeof(pcuda_worker_handle));
    if (handle == NULL) {
        delete worker;
        return OOM_ERROR;
    }
    if (worker->startThread()) {
        handle->ref = worker;
        ERL_NIF_TERM result = enif_make_resource(env, handle);
        enif_release_resource(handle);
        retval = enif_make_tuple2(env, ATOM_OK, result);
    }
    else {
        delete worker;
        enif_release_resource(handle);
        retval = ATOM_ERROR;
    }
    return retval;
}

ERL_NIF_TERM pteracuda_nifs_destroy_worker(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]) {
    pcuda_worker_handle *handle;
    if (argc != 1 || !enif_get_resource(env, argv[0], pteracuda_worker_resource, (void **) &handle)) {
        return enif_make_badarg(env);
    }
    PcudaWorker *worker = handle->ref;
    worker->stopThread();
    delete worker;
    return ATOM_OK;
}
