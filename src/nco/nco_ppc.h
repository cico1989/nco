/* $Header$ */

/* Purpose: Description (definition) of Precision-Preserving Compression (PPC) functions */

/* Copyright (C) 2015--2015 Charlie Zender
   This file is part of NCO, the netCDF Operators. NCO is free software.
   You may redistribute and/or modify NCO under the terms of the 
   GNU General Public License (GPL) Version 3 with exceptions described in the LICENSE file */

/* Usage:
   #include "nco_ppc.h" *//* Precision-Preserving Compression */

#ifndef NCO_PPC_H
#define NCO_PPC_H

/* Standard header files */
#include <ctype.h> /* isalnum(), isdigit(), tolower() */
#include <stdio.h> /* stderr, FILE, NULL, printf */
#include <stdlib.h> /* atof, atoi, malloc, getopt */
#include <string.h> /* strcmp() */

/* 3rd party vendors */
#include <netcdf.h> /* netCDF definitions and C library */
#include "nco_netcdf.h" /* NCO wrappers for netCDF C library */

/* Personal headers */
#include "nco.h" /* netCDF Operator (NCO) definitions */
#include "nco_mmr.h" /* Memory management */
#include "nco_sng_utl.h" /* String utilities */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  void 
  nco_ppc_ini /* [fnc] Set PPC based on user specifications */
  (const int nc_id, /* I [id] netCDF input file ID */
   int *dfl_lvl, /* O [enm] Deflate level */
   const int fl_out_fmt, /* I [enm] Output file format */
   char *const ppc_arg[], /* I [sng] List of user-specified ppc */
   const int ppc_nbr, /* I [nbr] Number of ppc specified */
   trv_tbl_sct * const trv_tbl); /* I/O [sct] Traversal table */
  
  void
  nco_ppc_att_prc /* [fnc] create ppc att from trv_tbl */
  (const int nc_id, /* I [id] Input netCDF file ID */
   const trv_tbl_sct * const trv_tbl); /* I [sct] GTT (Group Traversal Table) */
  
  void
  nco_ppc_set_dflt /* Set the ppc value for all non-coordinate vars */
  (const int nc_id, /* I [id] netCDF input file ID */
   const char * const ppc_arg, /* I [sng] user input for precision-preserving compression */
   trv_tbl_sct * const trv_tbl); /* I/O [sct] Traversal table */
  
  void
  nco_ppc_set_var
  (const char * const var_nm_fll, /* I [sng] Variable name to find */
   const char * const ppc_arg, /* I [sng] user input for precision-preserving compression */
   trv_tbl_sct * const trv_tbl); /* I/O [sct] Traversal table */

#ifdef __cplusplus
} /* end extern "C" */
#endif /* __cplusplus */

#endif /* NCO_PPC_H */
