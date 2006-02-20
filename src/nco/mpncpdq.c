/* $Header: /data/zender/nco_20150216/nco/src/nco/mpncpdq.c,v 1.34 2006-02-20 20:59:23 zender Exp $ */

/* mpncpdq -- netCDF pack, re-dimension, query */

/* Purpose: Pack, re-dimension, query single netCDF file and output to a single file */

/* Copyright (C) 1995--2006 Charlie Zender

   This software may be modified and/or re-distributed under the terms of the GNU General Public License (GPL) Version 2
   The full license text is at http://www.gnu.ai.mit.edu/copyleft/gpl.html 
   and in the file nco/doc/LICENSE in the NCO source distribution.

   As a special exception to the terms of the GPL, you are permitted 
   to link the NCO source code with the HDF, netCDF, OPeNDAP, and UDUnits
   libraries and to distribute the resulting executables under the terms 
   of the GPL, but in addition obeying the extra stipulations of the 
   HDF, netCDF, OPeNDAP, and UDUnits licenses.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
   See the GNU General Public License for more details.
   
   The original author of this software, Charlie Zender, wants to improve it
   with the help of your suggestions, improvements, bug-reports, and patches.
   Please contact the NCO project at http://nco.sf.net or write to
   Charlie Zender
   Department of Earth System Science
   University of California at Irvine
   Irvine, CA 92697-3100 */

/* Usage:
   ncpdq -O -D 3 -a lat,lev,lon -v three_dmn_var ~/nco/data/in.nc ~/foo.nc;ncks -P ~/foo.nc
   ncpdq -O -D 3 -a lon,lev,lat -v three_dmn_var ~/nco/data/in.nc ~/foo.nc;ncks -P ~/foo.nc
   ncpdq -O -D 3 -a lon,time -x -v three_double_dmn ~/nco/data/in.nc ~/foo.nc;ncks -P ~/foo.nc
   ncpdq -O -D 3 -P all_new ~/nco/data/in.nc ~/foo.nc
   ncpdq -O -D 3 -P all_xst ~/nco/data/in.nc ~/foo.nc
   ncpdq -O -D 3 -P xst_new ~/nco/data/in.nc ~/foo.nc
   ncpdq -O -D 3 -P upk ~/nco/data/in.nc ~/foo.nc */

#ifdef HAVE_CONFIG_H
#include <config.h> /* Autotools tokens */
#endif /* !HAVE_CONFIG_H */

/* Standard C headers */
#include <math.h> /* sin cos cos sin 3.14159 */
#include <stdio.h> /* stderr, FILE, NULL, etc. */
#include <stdlib.h> /* atof, atoi, malloc, getopt */
#include <string.h> /* strcmp. . . */
#include <time.h> /* machine time */
#include <unistd.h> /* all sorts of POSIX stuff */
#ifndef HAVE_GETOPT_LONG
#include "nco_getopt.h"
#else /* !NEED_GETOPT_LONG */ 
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* !HAVE_GETOPT_H */ 
#endif /* HAVE_GETOPT_LONG */

/* 3rd party vendors */
#include <netcdf.h> /* netCDF definitions and C library */
#ifdef ENABLE_MPI
#include <mpi.h> /* MPI definitions */
#include "nco_mpi.h" /* MPI utilities */
#endif /* !ENABLE_MPI */

/* #define MAIN_PROGRAM_FILE MUST precede #include libnco.h */
#define MAIN_PROGRAM_FILE
#include "libnco.h" /* netCDF Operator (NCO) library */

int 
main(int argc,char **argv)
{
  aed_sct *aed_lst_add_fst=NULL_CEWI;
  aed_sct *aed_lst_scl_fct=NULL_CEWI;
  
  nco_bool **dmn_rvr_in=NULL; /* [flg] Reverse dimension */
  nco_bool *dmn_rvr_rdr=NULL; /* [flg] Reverse dimension */
  nco_bool EXCLUDE_INPUT_LIST=False; /* Option c */
  nco_bool EXTRACT_ALL_COORDINATES=False; /* Option c */
  nco_bool EXTRACT_ASSOCIATED_COORDINATES=True; /* Option C */
  nco_bool FILE_RETRIEVED_FROM_REMOTE_LOCATION;
  nco_bool FL_LST_IN_FROM_STDIN=False; /* [flg] fl_lst_in comes from stdin */
  nco_bool FORCE_APPEND=False; /* Option A */
  nco_bool FORCE_OVERWRITE=False; /* Option O */
  nco_bool FORTRAN_IDX_CNV=False; /* Option F */
  nco_bool HISTORY_APPEND=True; /* Option h */
  nco_bool CNV_CCM_CCSM_CF;
  nco_bool REDEFINED_RECORD_DIMENSION=False; /* [flg] Re-defined record dimension */
  nco_bool REMOVE_REMOTE_FILES_AFTER_PROCESSING=True; /* Option R */
  
  char **dmn_rdr_lst_in=NULL_CEWI; /* Option a */
  char **fl_lst_abb=NULL; /* Option n */
  char **fl_lst_in=NULL_CEWI;
  char **var_lst_in=NULL_CEWI;
  char *cmd_ln;
  char *fl_in=NULL;
  char *fl_out=NULL; /* Option o */
  char *fl_out_tmp=NULL_CEWI;
  char *fl_pth=NULL; /* Option p */
  char *fl_pth_lcl=NULL; /* Option l */
  char *lmt_arg[NC_MAX_DIMS];
  char *nco_pck_plc_sng=NULL_CEWI; /* [sng] Packing policy Option P */
  char *nco_pck_map_sng=NULL_CEWI; /* [sng] Packing map Option M */
  char *opt_crr=NULL; /* [sng] String representation of current long-option name */
  char *optarg_lcl=NULL; /* [sng] Local copy of system optarg */
  char *rec_dmn_nm_in=NULL; /* [sng] Record dimension name, original */
  char *rec_dmn_nm_out=NULL; /* [sng] Record dimension name, re-ordered */
  char *rec_dmn_nm_out_crr=NULL; /* [sng] Name of record dimension, if any, required by re-order */
  char *time_bfr_srt;
  
  char add_fst_sng[]="add_offset"; /* [sng] Unidata standard string for add offset */
  char scl_fct_sng[]="scale_factor"; /* [sng] Unidata standard string for scale factor */
  
  const char * const CVS_Id="$Id: mpncpdq.c,v 1.34 2006-02-20 20:59:23 zender Exp $"; 
  const char * const CVS_Revision="$Revision: 1.34 $";
  const char * const opt_sht_lst="4Aa:CcD:d:Fhl:M:Oo:P:p:RrSt:v:Ux-:";
  
  dmn_sct **dim=NULL_CEWI;
  dmn_sct **dmn_out;
  dmn_sct **dmn_rdr=NULL; /* [sct] Dimension structures to be re-ordered */
  
  extern char *optarg;
  extern int optind;
  
  /* Using naked stdin/stdout/stderr in parallel region generates warning
     Copy appropriate filehandle to variable scoped shared in parallel clause */
  FILE * const fp_stderr=stderr; /* [fl] stderr filehandle CEWI */
  FILE * const fp_stdout=stdout; /* [fl] stdout filehandle CEWI */
  
  int **dmn_idx_out_in=NULL; /* [idx] Dimension correspondence, output->input CEWI */
  
  int *in_id_arr;

  int abb_arg_nbr=0;
  int dmn_out_idx; /* [idx] Index over output dimension list */
  int dmn_out_idx_rec_in=NCO_REC_DMN_UNDEFINED; /* [idx] Record dimension index in output dimension list, original */
  int dmn_rdr_nbr=0; /* [nbr] Number of dimension to re-order */
  int dmn_rdr_nbr_in=0; /* [nbr] Original number of dimension to re-order */
  int dmn_rdr_nbr_utl=0; /* [nbr] Number of dimension to re-order, utilized */
  int fl_idx=int_CEWI;
  int fl_nbr=0;
  int fl_out_fmt=NC_FORMAT_CLASSIC; /* [enm] Output file format */
  int fll_md_old; /* [enm] Old fill mode */
  int idx=int_CEWI;
  int idx_rdr=int_CEWI;
  int in_id;  
  int lmt_nbr=0; /* Option d. NB: lmt_nbr gets incremented */
  int nbr_dmn_fl;
  int nbr_dmn_out;
  int nbr_dmn_xtr;
  int nbr_var_fix; /* nbr_var_fix gets incremented */
  int nbr_var_fl;
  int nbr_var_prc; /* nbr_var_prc gets incremented */
  int nbr_xtr=0; /* nbr_xtr won't otherwise be set for -c with no -v */
  int nco_pck_map=nco_pck_map_flt_sht; /* [enm] Packing map */
  int nco_pck_plc=nco_pck_plc_nil; /* [enm] Packing policy */
  int opt;
  int out_id;  
  int rcd=NC_NOERR; /* [rcd] Return code */
  int rec_dmn_id_in=NCO_REC_DMN_UNDEFINED; /* [id] Record dimension ID in input file */
  int thr_idx; /* [idx] Index of current thread */
  int thr_nbr=int_CEWI; /* [nbr] Thread number Option t */
  int var_lst_in_nbr=0;
  
  lmt_sct **lmt;
  
  nm_id_sct *dmn_lst;
  nm_id_sct *dmn_rdr_lst;
  nm_id_sct *xtr_lst=NULL; /* xtr_lst may be alloc()'d from NULL with -c option */
  
  time_t time_crr_time_t;
  
  var_sct **var;
  var_sct **var_fix;
  var_sct **var_fix_out;
  var_sct **var_out;
  var_sct **var_prc;
  var_sct **var_prc_out;
  
#ifdef ENABLE_MPI
  /* Declare all MPI-specific variables here */
  MPI_Status mpi_stt; /* [enm] Status check to decode msg_tag_typ */

  nco_bool TKN_WRT_FREE=True; /* [flg] Write-access to output file is available */
  
  int fl_nm_lng; /* [nbr] Output file name length */
  int msg_bfr[msg_bfr_lng]; /* [bfr] Buffer containing var, idx, tkn_wrt_rsp */
  int jdx=0; /* [idx] For MPI indexing local variables */
  /* csz: fxm why 60? make dynamic? */
  int lcl_idx_lst[60]; /* [arr] Array containing indices of variables processed at each Worker */
  int lcl_nbr_var=0; /* [nbr] Count of variables processes at each Worker */
  int msg_tag_typ; /* [enm] MPI message tag type */
  int prc_rnk; /* [idx] Process rank */
  int prc_nbr=0; /* [nbr] Number of MPI processes */
  int tkn_wrt_rsp; /* [enm] Response to request for write token */
  int var_wrt_nbr=0; /* [nbr] Variables written to output file until now */
  int rnk_wrk; /* [idx] Worker rank */
  int wrk_id_bfr[wrk_id_bfr_lng]; /* [bfr] Buffer for rnk_wrk */
#endif /* !ENABLE_MPI */

  static struct option opt_lng[]=
    { /* Structure ordered by short option key if possible */
      /* Long options with no argument, no short option counterpart */
      /* Long options with argument, no short option counterpart */
      {"fl_fmt",required_argument,0,0},
      {"file_format",required_argument,0,0},
      /* Long options with short counterparts */
      {"4",no_argument,0,'4'},
      {"64bit",no_argument,0,'4'},
      {"netcdf4",no_argument,0,'4'},
      {"append",no_argument,0,'A'},
      {"arrange",required_argument,0,'a'},
      {"permute",required_argument,0,'a'},
      {"reorder",required_argument,0,'a'},
      {"rdr",required_argument,0,'a'},
      {"no-coords",no_argument,0,'C'},
      {"no-crd",no_argument,0,'C'},
      {"coords",no_argument,0,'c'},
      {"crd",no_argument,0,'c'},
      {"debug",required_argument,0,'D'},
      {"dbg_lvl",required_argument,0,'D'},
      {"dimension",required_argument,0,'d'},
      {"dmn",required_argument,0,'d'},
      {"fortran",no_argument,0,'F'},
      {"ftn",no_argument,0,'F'},
      {"history",no_argument,0,'h'},
      {"hst",no_argument,0,'h'},
      {"local",required_argument,0,'l'},
      {"lcl",required_argument,0,'l'},
      {"pck_map",required_argument,0,'M'},
      {"map",required_argument,0,'M'},
      {"overwrite",no_argument,0,'O'},
      {"ovr",no_argument,0,'O'},
      {"output",required_argument,0,'o'},
      {"fl_out",required_argument,0,'o'},
      {"pack_policy",required_argument,0,'P'},
      {"pck_plc",required_argument,0,'P'},
      {"path",required_argument,0,'p'},
      {"retain",no_argument,0,'R'},
      {"rtn",no_argument,0,'R'},
      {"revision",no_argument,0,'r'},
      {"version",no_argument,0,'r'},
      {"vrs",no_argument,0,'r'},
      {"suspend", no_argument,0,'S'},
      {"thr_nbr",required_argument,0,'t'},
      {"threads",required_argument,0,'t'},
      {"omp_num_threads",required_argument,0,'t'},
      {"unpack",no_argument,0,'U'},
      {"upk",no_argument,0,'U'},
      {"variable",required_argument,0,'v'},
      {"exclude",no_argument,0,'x'},
      {"xcl",no_argument,0,'x'},
      {"help",no_argument,0,'?'},
      {0,0,0,0}
    }; /* end opt_lng */
  int opt_idx=0; /* Index of current long option into opt_lng array */
  
#ifdef ENABLE_MPI
  /* MPI Initialization */
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&prc_nbr);
  MPI_Comm_rank(MPI_COMM_WORLD,&prc_rnk);
#endif /* !ENABLE_MPI */
  
  /* Start clock and save command line */ 
  cmd_ln=nco_cmd_ln_sng(argc,argv);
  time_crr_time_t=time((time_t *)NULL);
  time_bfr_srt=ctime(&time_crr_time_t); time_bfr_srt=time_bfr_srt; /* Avoid compiler warning until variable is used for something */
  
  time_bfr_srt=time_bfr_srt; /* CEWI: Avert compiler warning that variable is set but never used */
  
  /* Get program name and set program enum (e.g., prg=ncra) */
  prg_nm=prg_prs(argv[0],&prg);
  
  /* Parse command line arguments */
  while(1){
    /* getopt_long_only() allows one dash to prefix long options */
    opt=getopt_long(argc,argv,opt_sht_lst,opt_lng,&opt_idx);
    /* NB: access to opt_crr is only valid when long_opt is detected */
    if(opt == EOF) break; /* Parse positional arguments once getopt_long() returns EOF */
    opt_crr=(char *)strdup(opt_lng[opt_idx].name);

    /* Process long options without short option counterparts */
    if(opt == 0){
      if(!strcmp(opt_crr,"fl_fmt") || !strcmp(opt_crr,"file_format")) rcd=nco_create_mode_prs(optarg,&fl_out_fmt);
    } /* opt != 0 */
    /* Process short options */
    switch(opt){
    case 0: /* Long options have already been processed, return */
      break;
    case '4': /* [flg] Catch-all to prescribe output storage format */
      if(!strcmp(opt_crr,"64bit")) fl_out_fmt=NC_FORMAT_64BIT; else fl_out_fmt=NC_FORMAT_NETCDF4; 
      break;
    case 'A': /* Toggle FORCE_APPEND */
      FORCE_APPEND=!FORCE_APPEND;
      break;
    case 'a': /* Re-order dimensions */
      dmn_rdr_lst_in=lst_prs_2D(optarg,",",&dmn_rdr_nbr_in);
      dmn_rdr_nbr=dmn_rdr_nbr_in;
      break;
    case 'C': /* Extract all coordinates associated with extracted variables? */
      EXTRACT_ASSOCIATED_COORDINATES=False;
      break;
    case 'c':
      EXTRACT_ALL_COORDINATES=True;
      break;
    case 'D': /* Debugging level. Default is 0. */
      dbg_lvl=(unsigned short)strtol(optarg,(char **)NULL,10);
      break;
    case 'd': /* Copy argument for later processing */
      lmt_arg[lmt_nbr]=(char *)strdup(optarg);
      lmt_nbr++;
      break;
    case 'F': /* Toggle index convention. Default is 0-based arrays (C-style). */
      FORTRAN_IDX_CNV=!FORTRAN_IDX_CNV;
      break;
    case 'h': /* Toggle appending to history global attribute */
      HISTORY_APPEND=!HISTORY_APPEND;
      break;
    case 'l': /* Local path prefix for files retrieved from remote file system */
      fl_pth_lcl=(char *)strdup(optarg);
      break;
    case 'M': /* Packing map */
      nco_pck_map_sng=(char *)strdup(optarg);
      nco_pck_map=nco_pck_map_get(nco_pck_map_sng);
      break;
    case 'O': /* Toggle FORCE_OVERWRITE */
      FORCE_OVERWRITE=!FORCE_OVERWRITE;
      break;
    case 'o': /* Name of output file */
      fl_out=(char *)strdup(optarg);
      break;
    case 'P': /* Packing policy */
      nco_pck_plc_sng=(char *)strdup(optarg);
      break;
    case 'p': /* Common file path */
      fl_pth=(char *)strdup(optarg);
      break;
    case 'R': /* Toggle removal of remotely-retrieved-files. Default is True. */
      REMOVE_REMOTE_FILES_AFTER_PROCESSING=!REMOVE_REMOTE_FILES_AFTER_PROCESSING;
      break;
    case 'r': /* Print CVS program information and copyright notice */
      (void)copyright_prn(CVS_Id,CVS_Revision);
      (void)nco_lbr_vrs_prn();
      nco_exit(EXIT_SUCCESS);
      break;
#ifdef ENABLE_MPI
    case 'S': /* Suspend with signal handler to facilitate debugging */
      if(signal(SIGUSR1,nco_cnt_run) == SIG_ERR) (void)fprintf(fp_stdout,"%s: ERROR Could not install suspend handler.\n",prg_nm);
      while(!nco_spn_lck_brk) usleep(nco_spn_lck_us); /* Spinlock. fxm: should probably insert a sched_yield */
      break;
#endif /* !ENABLE_MPI */
    case 't': /* Thread number */
      thr_nbr=(int)strtol(optarg,(char **)NULL,10);
      break;
    case 'U': /* Unpacking switch */
      nco_pck_plc_sng=(char *)strdup("upk");
      break;
    case 'v': /* Variables to extract/exclude */
      /* Replace commas with hashes when within braces (convert back later) */
      optarg_lcl=(char *)strdup(optarg);
      (void)nco_lst_comma2hash(optarg_lcl);
      var_lst_in=lst_prs_2D(optarg_lcl,",",&var_lst_in_nbr);
      optarg_lcl=(char *)nco_free(optarg_lcl);
      nbr_xtr=var_lst_in_nbr;
      break;
    case 'x': /* Exclude rather than extract variables specified with -v */
      EXCLUDE_INPUT_LIST=True;
      break;
    case '?': /* Print proper usage */
      (void)nco_usg_prn();
      nco_exit(EXIT_SUCCESS);
      break;
    case '-': /* Long options are not allowed */
      (void)fprintf(stderr,"%s: ERROR Long options are not available in this build. Use single letter options instead.\n",prg_nm_get());
      nco_exit(EXIT_FAILURE);
      break;
    default: /* Print proper usage */
      (void)nco_usg_prn();
      nco_exit(EXIT_FAILURE);
      break;
    } /* end switch */
    if(opt_crr != NULL) opt_crr=(char *)nco_free(opt_crr);
  } /* end while loop */
  
  /* Process positional arguments and fill in filenames */
  fl_lst_in=nco_fl_lst_mk(argv,argc,optind,&fl_nbr,&fl_out,&FL_LST_IN_FROM_STDIN);
  
  /* Make uniform list of user-specified dimension limits */
  lmt=nco_lmt_prs(lmt_nbr,lmt_arg);
    
  /* Initialize thread information */
  thr_nbr=nco_openmp_ini(thr_nbr);
  in_id_arr=(int *)nco_malloc(thr_nbr*sizeof(int));

  /* Parse filename */
  fl_in=nco_fl_nm_prs(fl_in,0,&fl_nbr,fl_lst_in,abb_arg_nbr,fl_lst_abb,fl_pth);
  /* Make sure file is on local system and is readable or die trying */
  fl_in=nco_fl_mk_lcl(fl_in,fl_pth_lcl,&FILE_RETRIEVED_FROM_REMOTE_LOCATION);
  /* Open file for reading */
  rcd=nco_open(fl_in,NC_NOWRITE,&in_id);
  
  /* Get number of variables, dimensions, and record dimension ID of input file */
  (void)nco_inq(in_id,&nbr_dmn_fl,&nbr_var_fl,(int *)NULL,&rec_dmn_id_in);
  
  /* Form initial extraction list which may include extended regular expressions */
  xtr_lst=nco_var_lst_mk(in_id,nbr_var_fl,var_lst_in,EXTRACT_ALL_COORDINATES,&nbr_xtr);
  
  /* Change included variables to excluded variables */
  if(EXCLUDE_INPUT_LIST) xtr_lst=nco_var_lst_xcl(in_id,nbr_var_fl,xtr_lst,&nbr_xtr);
  
  /* Add all coordinate variables to extraction list */
  if(EXTRACT_ALL_COORDINATES) xtr_lst=nco_var_lst_add_crd(in_id,nbr_dmn_fl,xtr_lst,&nbr_xtr);
  
  /* Make sure coordinates associated extracted variables are also on extraction list */
  if(EXTRACT_ASSOCIATED_COORDINATES) xtr_lst=nco_var_lst_ass_crd_add(in_id,xtr_lst,&nbr_xtr);
  
  /* Sort extraction list by variable ID for fastest I/O */
  if(nbr_xtr > 1) xtr_lst=nco_lst_srt_nm_id(xtr_lst,nbr_xtr,False);
  
  /* Find coordinate/dimension values associated with user-specified limits
     NB: nco_lmt_evl() with same nc_id contains OpenMP critical region */
  for(idx=0;idx<lmt_nbr;idx++) (void)nco_lmt_evl(in_id,lmt[idx],0L,FORTRAN_IDX_CNV);
  
  /* Find dimensions associated with variables to be extracted */
  dmn_lst=nco_dmn_lst_ass_var(in_id,xtr_lst,nbr_xtr,&nbr_dmn_xtr);
  
  /* Fill in dimension structure for all extracted dimensions */
  dim=(dmn_sct **)nco_malloc(nbr_dmn_xtr*sizeof(dmn_sct *));
  for(idx=0;idx<nbr_dmn_xtr;idx++) dim[idx]=nco_dmn_fll(in_id,dmn_lst[idx].id,dmn_lst[idx].nm);
  /* Dimension list no longer needed */
  dmn_lst=nco_nm_id_lst_free(dmn_lst,nbr_dmn_xtr);
  
  /* Merge hyperslab limit information into dimension structures */
  if(lmt_nbr > 0) (void)nco_dmn_lmt_mrg(dim,nbr_dmn_xtr,lmt,lmt_nbr);
  
  /* Duplicate input dimension structures for output dimension structures */
  nbr_dmn_out=nbr_dmn_xtr;
  dmn_out=(dmn_sct **)nco_malloc(nbr_dmn_out*sizeof(dmn_sct *));
  for(idx=0;idx<nbr_dmn_out;idx++){
    dmn_out[idx]=nco_dmn_dpl(dim[idx]);
    (void)nco_dmn_xrf(dim[idx],dmn_out[idx]);
  } /* end loop over idx */
  
  /* No re-order dimensions specified implies packing request */
  if(dmn_rdr_nbr == 0){
    if(nco_pck_plc == nco_pck_plc_nil) nco_pck_plc=nco_pck_plc_get(nco_pck_plc_sng);
    if(dbg_lvl > 0) (void)fprintf(stderr,"%s: DEBUG Packing map is %s and packing policy is %s\n",prg_nm_get(),nco_pck_map_sng_get(nco_pck_map),nco_pck_plc_sng_get(nco_pck_plc));
  } /* endif */
  
  /* From this point forward, assume ncpdq operator packs or re-orders, not both */
  if(dmn_rdr_nbr > 0 && nco_pck_plc != nco_pck_plc_nil){
    (void)fprintf(fp_stdout,"%s: ERROR %s does not support simultaneous dimension re-ordering  (-a switch) and packing (-P switch).\nHINT: Invoke %s twice, once to re-order (with -a), and once to pack (with -P).\n",prg_nm,prg_nm,prg_nm);
    nco_exit(EXIT_FAILURE);
  } /* end if */
  
  if(dmn_rdr_nbr > 0){
    /* NB: Same logic as in ncwa, perhaps combine into single function, nco_dmn_avg_rdr_prp()? */
    /* Make list of user-specified dimension re-orders */
    
    /* Create reversed dimension list */
    dmn_rvr_rdr=(nco_bool *)nco_malloc(dmn_rdr_nbr*sizeof(nco_bool));
    for(idx_rdr=0;idx_rdr<dmn_rdr_nbr;idx_rdr++){
      if(dmn_rdr_lst_in[idx_rdr][0] == '-'){
	dmn_rvr_rdr[idx_rdr]=True;
	/* Copy string to new memory one past negative sign to avoid losing byte */
	optarg_lcl=dmn_rdr_lst_in[idx_rdr];
	dmn_rdr_lst_in[idx_rdr]=(char *)strdup(optarg_lcl+1);
	optarg_lcl=(char *)nco_free(optarg_lcl);
      }else{
	dmn_rvr_rdr[idx_rdr]=False;
      } /* end else */
    } /* end loop over idx_rdr */
    
    /* Create structured list of re-ordering dimension names and IDs */
    dmn_rdr_lst=nco_dmn_lst_mk(in_id,dmn_rdr_lst_in,dmn_rdr_nbr);
    
    /* Form list of re-ordering dimensions from extracted input dimensions */
    dmn_rdr=(dmn_sct **)nco_malloc(dmn_rdr_nbr*sizeof(dmn_sct *));
    /* Loop over original number of re-order dimensions */
    for(idx_rdr=0;idx_rdr<dmn_rdr_nbr;idx_rdr++){
      for(idx=0;idx<nbr_dmn_xtr;idx++){
	if(!strcmp(dmn_rdr_lst[idx_rdr].nm,dim[idx]->nm)) break;
      } /* end loop over idx_rdr */
      if(idx != nbr_dmn_xtr) dmn_rdr[dmn_rdr_nbr_utl++]=dim[idx]; else (void)fprintf(stderr,"%s: WARNING re-ordering dimension \"%s\" is not contained in any variable in extraction list\n",prg_nm,dmn_rdr_lst[idx_rdr].nm);
    } /* end loop over idx_rdr */
    dmn_rdr_nbr=dmn_rdr_nbr_utl;
    /* Collapse extra dimension structure space to prevent accidentally using it */
    dmn_rdr=(dmn_sct **)nco_realloc(dmn_rdr,dmn_rdr_nbr*sizeof(dmn_sct *));
    /* Dimension list in name-ID format is no longer needed */
    dmn_rdr_lst=nco_nm_id_lst_free(dmn_rdr_lst,dmn_rdr_nbr);
    
    /* Make sure no re-ordering dimension is specified more than once */
    for(idx=0;idx<dmn_rdr_nbr;idx++){
      for(idx_rdr=0;idx_rdr<dmn_rdr_nbr;idx_rdr++){
	if(idx_rdr != idx){
	  if(dmn_rdr[idx]->id == dmn_rdr[idx_rdr]->id){
	    (void)fprintf(fp_stdout,"%s: ERROR %s specified more than once in reducing list\n",prg_nm,dmn_rdr[idx]->nm);
	    nco_exit(EXIT_FAILURE);
	  } /* end if */
	} /* end if */
      } /* end loop over idx_rdr */
    } /* end loop over idx */
    
    if(dmn_rdr_nbr > nbr_dmn_xtr){
      (void)fprintf(fp_stdout,"%s: ERROR More re-ordering dimensions than extracted dimensions\n",prg_nm);
      nco_exit(EXIT_FAILURE);
    } /* end if */
    
  } /* dmn_rdr_nbr <= 0 */
  
  /* Is this an CCM/CCSM/CF-format history tape? */
  CNV_CCM_CCSM_CF=nco_cnv_ccm_ccsm_cf_inq(in_id);
  
  /* Fill in variable structure list for all extracted variables */
  var=(var_sct **)nco_malloc(nbr_xtr*sizeof(var_sct *));
  var_out=(var_sct **)nco_malloc(nbr_xtr*sizeof(var_sct *));
  for(idx=0;idx<nbr_xtr;idx++){
    var[idx]=nco_var_fll(in_id,xtr_lst[idx].id,xtr_lst[idx].nm,dim,nbr_dmn_xtr);
    var_out[idx]=nco_var_dpl(var[idx]);
    (void)nco_xrf_var(var[idx],var_out[idx]);
    (void)nco_xrf_dmn(var_out[idx]);
  } /* end loop over idx */
  /* Extraction list no longer needed */
  xtr_lst=nco_nm_id_lst_free(xtr_lst,nbr_xtr);
  
  /* Divide variable lists into lists of fixed variables and variables to be processed */
  (void)nco_var_lst_dvd(var,var_out,nbr_xtr,CNV_CCM_CCSM_CF,nco_pck_map,nco_pck_plc,dmn_rdr,dmn_rdr_nbr,&var_fix,&var_fix_out,&nbr_var_fix,&var_prc,&var_prc_out,&nbr_var_prc);
  
  /* We now have final list of variables to extract. Phew. */
  if(dbg_lvl > 2){
    for(idx=0;idx<nbr_xtr;idx++) (void)fprintf(stderr,"var[%d]->nm = %s, ->id=[%d]\n",idx,var[idx]->nm,var[idx]->id);
    for(idx=0;idx<nbr_var_fix;idx++) (void)fprintf(stderr,"var_fix[%d]->nm = %s, ->id=[%d]\n",idx,var_fix[idx]->nm,var_fix[idx]->id);
    for(idx=0;idx<nbr_var_prc;idx++) (void)fprintf(stderr,"var_prc[%d]->nm = %s, ->id=[%d]\n",idx,var_prc[idx]->nm,var_prc[idx]->id);
  } /* end if */
  
#ifdef ENABLE_MPI
  if(prc_rnk == rnk_mgr){ /* MPI manager code */
#endif /* !ENABLE_MPI */
    /* Open output file */
    fl_out_tmp=nco_fl_out_open(fl_out,FORCE_APPEND,FORCE_OVERWRITE,fl_out_fmt,&out_id);
    if(dbg_lvl > 4) (void)fprintf(stderr,"Input, output file IDs = %d, %d\n",in_id,out_id);
    
    /* Copy global attributes */
    (void)nco_att_cpy(in_id,out_id,NC_GLOBAL,NC_GLOBAL,True);
    
    /* Catenate time-stamped command line to "history" global attribute */
    if(HISTORY_APPEND) (void)nco_hst_att_cat(out_id,cmd_ln);
    
    if(thr_nbr > 0 && HISTORY_APPEND) (void)nco_thr_att_cat(out_id,thr_nbr);
    
#ifdef ENABLE_MPI
    /* Initialize MPI task information */
    if(prc_nbr > 0 && HISTORY_APPEND) (void)nco_mpi_att_cat(out_id,prc_nbr);
  } /* !prc_rnk == rnk_mgr */
#endif /* !ENABLE_MPI */
  
  /* If re-ordering, then in files with record dimension... */
  if(dmn_rdr_nbr > 0 && rec_dmn_id_in != NCO_REC_DMN_UNDEFINED){
    /* ...which, if any, output dimension structure currently holds record dimension? */
    for(dmn_out_idx=0;dmn_out_idx<nbr_dmn_out;dmn_out_idx++)
      if(dmn_out[dmn_out_idx]->is_rec_dmn) break;
    if(dmn_out_idx != nbr_dmn_out){
      dmn_out_idx_rec_in=dmn_out_idx;
      /* Initialize output record dimension to input record dimension */
      rec_dmn_nm_in=rec_dmn_nm_out=dmn_out[dmn_out_idx_rec_in]->nm;
    }else{
      dmn_out_idx_rec_in=NCO_REC_DMN_UNDEFINED;
    } /* end else */
  } /* end if file contains record dimension */
  
  /* If re-ordering, determine and set new dimensionality in metadata of each re-ordered variable */
  if(dmn_rdr_nbr > 0){
    dmn_idx_out_in=(int **)nco_malloc(nbr_var_prc*sizeof(int *));
    dmn_rvr_in=(nco_bool **)nco_malloc(nbr_var_prc*sizeof(nco_bool *));
    for(idx=0;idx<nbr_var_prc;idx++){
      dmn_idx_out_in[idx]=(int *)nco_malloc(var_prc[idx]->nbr_dim*sizeof(int));
      dmn_rvr_in[idx]=(nco_bool *)nco_malloc(var_prc[idx]->nbr_dim*sizeof(nco_bool));
      /* nco_var_dmn_rdr_mtd() does re-order heavy lifting */
      rec_dmn_nm_out_crr=nco_var_dmn_rdr_mtd(var_prc[idx],var_prc_out[idx],dmn_rdr,dmn_rdr_nbr,dmn_idx_out_in[idx],dmn_rvr_rdr,dmn_rvr_in[idx]);
      /* If record dimension required by current variable re-order...
	 ...and variable is multi-dimensional (one dimensional arrays
	 cannot request record dimension changes)... */
      if(rec_dmn_nm_out_crr && var_prc_out[idx]->nbr_dim > 1){
	/* ...differs from input and current output record dimension(s)... */
	if(strcmp(rec_dmn_nm_out_crr,rec_dmn_nm_in) && strcmp(rec_dmn_nm_out_crr,rec_dmn_nm_out)){
	  /* ...and current output record dimension already differs from input record dimension... */
	  if(REDEFINED_RECORD_DIMENSION){
	    /* ...then requested re-order requires multiple record dimensions... */
	    (void)fprintf(fp_stdout,"%s: WARNING Re-order requests multiple record dimensions\n. Only first request will be honored (netCDF allows only one record dimension). Record dimensions involved [original,first change request (honored),latest change request (made by variable %s)]=[%s,%s,%s]\n",prg_nm,var_prc[idx]->nm,rec_dmn_nm_in,rec_dmn_nm_out,rec_dmn_nm_out_crr);
	    break;
	  }else{ /* !REDEFINED_RECORD_DIMENSION */
	    /* ...otherwise, update output record dimension name... */
	    rec_dmn_nm_out=rec_dmn_nm_out_crr;
	    /* ...and set new and un-set old record dimensions... */
	    var_prc_out[idx]->dim[0]->is_rec_dmn=True;
	    dmn_out[dmn_out_idx_rec_in]->is_rec_dmn=False;
	    /* ...and set flag that record dimension has been re-defined... */
	    REDEFINED_RECORD_DIMENSION=True;
	  } /* !REDEFINED_RECORD_DIMENSION */
	} /* endif new and old record dimensions differ */
      } /* endif current variable is record variable */
    } /* end loop over var_prc */
  } /* endif dmn_rdr_nbr > 0 */
  
  /* NB: Much of following logic is required by netCDF constraint that only
     one record variable is allowed per file. netCDF4 will relax this constraint.
     Hence making following logic prettier or funcionalizing is not high priority.
     Logic may need to be simplified/re-written once netCDF4 is released. */
  if(REDEFINED_RECORD_DIMENSION){
    (void)fprintf(fp_stdout,"%s: INFO Requested re-order will change record dimension from %s to %s. netCDF allows only one record dimension. Hence %s will make %s record (least rapidly varying) dimension in all variables that contain it.\n",prg_nm,rec_dmn_nm_in,rec_dmn_nm_out,prg_nm,rec_dmn_nm_out);
    /* Changing record dimension may invalidate is_rec_var flag
       Updating is_rec_var flag to correct value, even if value is ignored,
       helps keep user appraised of unexpected dimension re-orders.
       is_rec_var may change both for "fixed" and "processed" variables
       When is_rec_var changes for processed variables, may also need to change
       ancillary information and to check for duplicate dimensions.
       Ancillary information (dmn_idx_out_in) is available only for var_prc!
       Hence must update is_rec_var flag for var_fix and var_prc separately */
    
    /* Update is_rec_var flag for var_fix */
    for(idx=0;idx<nbr_var_fix;idx++){
      /* Search all dimensions in variable for new record dimension */
      for(dmn_out_idx=0;dmn_out_idx<var_fix[idx]->nbr_dim;dmn_out_idx++)
	if(!strcmp(var_fix[idx]->dim[dmn_out_idx]->nm,rec_dmn_nm_out)) break;
      /* ...Will variable be record variable in output file?... */
      if(dmn_out_idx == var_fix[idx]->nbr_dim){
	/* ...No. Variable will be non-record---does this change its status?... */
	if(dbg_lvl > 2) if(var_fix[idx]->is_rec_var == True) (void)fprintf(fp_stdout,"%s: INFO Requested re-order will change variable %s from record to non-record variable\n",prg_nm,var_fix[idx]->nm);
	/* Assign record flag dictated by re-order */
	var_fix[idx]->is_rec_var=False; 
      }else{ /* ...otherwise variable will be record variable... */
	/* ...Yes. Variable will be record... */
	/* ...Will becoming record variable change its status?... */
	if(var_fix[idx]->is_rec_var == False){
	  if(dbg_lvl > 2) (void)fprintf(fp_stdout,"%s: INFO Requested re-order will change variable %s from non-record to record variable\n",prg_nm,var_fix[idx]->nm);
	  /* Change record flag to status dictated by re-order */
	  var_fix[idx]->is_rec_var=True;
	} /* endif status changing from non-record to record */
      } /* endif variable will be record variable */
    } /* end loop over var_fix */
    
    /* Update is_rec_var flag for var_prc */
    for(idx=0;idx<nbr_var_prc;idx++){
      /* Search all dimensions in variable for new record dimension */
      for(dmn_out_idx=0;dmn_out_idx<var_prc_out[idx]->nbr_dim;dmn_out_idx++)
	if(!strcmp(var_prc_out[idx]->dim[dmn_out_idx]->nm,rec_dmn_nm_out)) break;
      /* ...Will variable be record variable in output file?... */
      if(dmn_out_idx == var_prc_out[idx]->nbr_dim){
	/* ...No. Variable will be non-record---does this change its status?... */
	if(dbg_lvl > 2) if(var_prc_out[idx]->is_rec_var == True) (void)fprintf(fp_stdout,"%s: INFO Requested re-order will change variable %s from record to non-record variable\n",prg_nm,var_prc_out[idx]->nm);
	/* Assign record flag dictated by re-order */
	var_prc_out[idx]->is_rec_var=False; 
      }else{ /* ...otherwise variable will be record variable... */
	/* ...Yes. Variable will be record... */
	/* ...must ensure new record dimension is not duplicate dimension... */
	if(var_prc_out[idx]->has_dpl_dmn){
	  int dmn_dpl_idx;
	  for(dmn_dpl_idx=1;dmn_dpl_idx<var_prc_out[idx]->nbr_dim;dmn_dpl_idx++){ /* NB: loop starts from 1 */
	    if(var_prc_out[idx]->dmn_id[0] == var_prc_out[idx]->dmn_id[dmn_dpl_idx]){
	      (void)fprintf(stdout,"%s: ERROR Requested re-order turns duplicate non-record dimension %s in variable %s into output record dimension. netCDF does not support duplicate record dimensions in a single variable.\n%s: HINT: Exclude variable %s from extraction list with \"-x -v %s\".\n",prg_nm_get(),rec_dmn_nm_out,var_prc_out[idx]->nm,prg_nm_get(),var_prc_out[idx]->nm,var_prc_out[idx]->nm);
	      nco_exit(EXIT_FAILURE);
	    } /* endif err */
	  } /* end loop over dmn_out */
	} /* endif has_dpl_dmn */
	/* ...Will becoming record variable change its status?... */
	if(var_prc_out[idx]->is_rec_var == False){
	  if(dbg_lvl > 2) (void)fprintf(fp_stdout,"%s: INFO Requested re-order will change variable %s from non-record to record variable\n",prg_nm,var_prc_out[idx]->nm);
	  /* Change record flag to status dictated by re-order */
	  var_prc_out[idx]->is_rec_var=True;
	  /* ...Swap dimension information for multi-dimensional variables... */
	  if(var_prc_out[idx]->nbr_dim > 1){
	    /* Swap dimension information when turning multi-dimensional 
	       non-record variable into record variable. 
	       Single dimensional non-record variables that turn into 
	       record variables already have correct dimension information */
	    dmn_sct *dmn_swp; /* [sct] Dimension structure for swapping */
	    int dmn_idx_rec_in; /* [idx] Record dimension index in input variable */
	    int dmn_idx_rec_out; /* [idx] Record dimension index in output variable */
	    int dmn_idx_swp; /* [idx] Dimension index for swapping */
	    /* If necessary, swap new record dimension to first position */
	    /* Label indices with standard names */
	    dmn_idx_rec_in=dmn_out_idx;
	    dmn_idx_rec_out=0;
	    /* Swap indices in map */
	    dmn_idx_swp=dmn_idx_out_in[idx][dmn_idx_rec_out];
	    dmn_idx_out_in[idx][dmn_idx_rec_out]=dmn_idx_rec_in;
	    dmn_idx_out_in[idx][dmn_idx_rec_in]=dmn_idx_swp;
	    /* Swap dimensions in list */
	    dmn_swp=var_prc_out[idx]->dim[dmn_idx_rec_out];
	    var_prc_out[idx]->dim[dmn_idx_rec_out]=var_prc_out[idx]->dim[dmn_idx_rec_in];
	    var_prc_out[idx]->dim[dmn_idx_rec_in]=dmn_swp;
	    /* NB: Change dmn_id,cnt,srt,end,srd together to minimize chances of forgetting one */
	    /* Correct output variable structure copy of output record dimension information */
	    var_prc_out[idx]->dmn_id[dmn_idx_rec_out]=var_prc_out[idx]->dim[dmn_idx_rec_out]->id;
	    var_prc_out[idx]->cnt[dmn_idx_rec_out]=var_prc_out[idx]->dim[dmn_idx_rec_out]->cnt;
	    var_prc_out[idx]->srt[dmn_idx_rec_out]=var_prc_out[idx]->dim[dmn_idx_rec_out]->srt;
	    var_prc_out[idx]->end[dmn_idx_rec_out]=var_prc_out[idx]->dim[dmn_idx_rec_out]->end;
	    var_prc_out[idx]->srd[dmn_idx_rec_out]=var_prc_out[idx]->dim[dmn_idx_rec_out]->srd;
	    /* Correct output variable structure copy of input record dimension information */
	    var_prc_out[idx]->dmn_id[dmn_idx_rec_in]=var_prc_out[idx]->dim[dmn_idx_rec_in]->id;
	    var_prc_out[idx]->cnt[dmn_idx_rec_in]=var_prc_out[idx]->dim[dmn_idx_rec_in]->cnt;
	    var_prc_out[idx]->srt[dmn_idx_rec_in]=var_prc_out[idx]->dim[dmn_idx_rec_in]->srt;
	    var_prc_out[idx]->end[dmn_idx_rec_in]=var_prc_out[idx]->dim[dmn_idx_rec_in]->end;
	    var_prc_out[idx]->srd[dmn_idx_rec_in]=var_prc_out[idx]->dim[dmn_idx_rec_in]->srd;
	  } /* endif multi-dimensional */
	} /* endif status changing from non-record to record */
      } /* endif variable will be record variable */
    } /* end loop over var_prc */
  } /* !REDEFINED_RECORD_DIMENSION */
  
#ifdef ENABLE_MPI
  if(prc_rnk == rnk_mgr){ /* Defining dimension in output file done by Manager alone */
#endif /* !ENABLE_MPI */
    /* Once new record dimension, if any, is known, define dimensions in output file */
    (void)nco_dmn_dfn(fl_out,out_id,dmn_out,nbr_dmn_out);
#ifdef ENABLE_MPI
  } /* prc_rnk != rnk_mgr */
#endif /* !ENABLE_MPI */
  
  /* Alter metadata for variables that will be packed */
  if(nco_pck_plc != nco_pck_plc_nil){
    if(nco_pck_plc != nco_pck_plc_upk){
      /* Allocate attribute list container for maximum number of entries */
      aed_lst_add_fst=(aed_sct *)nco_malloc(nbr_var_prc*sizeof(aed_sct));
      aed_lst_scl_fct=(aed_sct *)nco_malloc(nbr_var_prc*sizeof(aed_sct));
    } /* endif packing */
    for(idx=0;idx<nbr_var_prc;idx++){
      nco_pck_mtd(var_prc[idx],var_prc_out[idx],nco_pck_map,nco_pck_plc);
      if(nco_pck_plc != nco_pck_plc_upk){
	/* Use same copy of attribute name for all edits */
	aed_lst_add_fst[idx].att_nm=add_fst_sng;
	aed_lst_scl_fct[idx].att_nm=scl_fct_sng;
      } /* endif packing */
    } /* end loop over var_prc */
  } /* nco_pck_plc == nco_pck_plc_nil */
  
#ifdef ENABLE_MPI
  if(prc_rnk == rnk_mgr){ /* MPI manager code */
#endif /* !ENABLE_MPI */
    /* Define variables in output file, copy their attributes */
    (void)nco_var_dfn(in_id,fl_out,out_id,var_out,nbr_xtr,(dmn_sct **)NULL,(int)0,nco_pck_map,nco_pck_plc);
    
    /* Turn off default filling behavior to enhance efficiency */
    rcd=nco_set_fill(out_id,NC_NOFILL,&fll_md_old);
    
    /* Take output file out of define mode */
    (void)nco_enddef(out_id);
#ifdef ENABLE_MPI
  } /* prc_rnk != rnk_mgr */
  
  /* Manager obtains output filename and broadcasts to workers */
  if(prc_rnk == rnk_mgr) fl_nm_lng=(int)strlen(fl_out_tmp); 
  MPI_Bcast(&fl_nm_lng,1,MPI_INT,0,MPI_COMM_WORLD);
  if(prc_rnk != rnk_mgr) fl_out_tmp=(char *)malloc((fl_nm_lng+1)*sizeof(char));
  MPI_Bcast(fl_out_tmp,fl_nm_lng+1,MPI_CHAR,0,MPI_COMM_WORLD);
  
#endif /* !ENABLE_MPI */
  
  /* Zero start vectors for all output variables */
  (void)nco_var_srt_zero(var_out,nbr_xtr);
  
#ifdef ENABLE_MPI
  if(prc_rnk == rnk_mgr){ /* MPI manager code */
    TKN_WRT_FREE=False;
#endif /* !ENABLE_MPI */
    /* Copy variable data for non-processed variables */
    (void)nco_var_val_cpy(in_id,out_id,var_fix,nbr_var_fix);
#ifdef ENABLE_MPI
    /* Close output file so workers can open it */
    nco_close(out_id);
    TKN_WRT_FREE=True;
  } /* prc_rnk != rnk_mgr */
#endif /* !ENABLE_MPI */
  
  /* Close first input netCDF file */
  nco_close(in_id);
  
  /* Loop over input files (not currently used, fl_nbr == 1) */
  for(fl_idx=0;fl_idx<fl_nbr;fl_idx++){
    /* Parse filename */
    if(fl_idx != 0) fl_in=nco_fl_nm_prs(fl_in,fl_idx,&fl_nbr,fl_lst_in,abb_arg_nbr,fl_lst_abb,fl_pth);
    if(dbg_lvl > 0) (void)fprintf(stderr,"\nInput file %d is %s; ",fl_idx,fl_in);
    /* Make sure file is on local system and is readable or die trying */
    if(fl_idx != 0) fl_in=nco_fl_mk_lcl(fl_in,fl_pth_lcl,&FILE_RETRIEVED_FROM_REMOTE_LOCATION);
    if(dbg_lvl > 0) (void)fprintf(stderr,"local file %s:\n",fl_in);
    
    /* Open file once per thread to improve caching */
    for(thr_idx=0;thr_idx<thr_nbr;thr_idx++) rcd=nco_open(fl_in,NC_NOWRITE,in_id_arr+thr_idx);
    
#ifdef ENABLE_MPI
    if(prc_rnk == rnk_mgr){ /* MPI manager code */
      /* Compensate for incrementing on each worker's first message */
      var_wrt_nbr=-prc_nbr+1;
      idx=0;
      /* While variables remain to be processed or written... */
      while(var_wrt_nbr < nbr_var_prc){
        /* Receive message from any worker */
        MPI_Recv(wrk_id_bfr,wrk_id_bfr_lng,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&mpi_stt);
        /* Obtain MPI message tag type */
        msg_tag_typ=mpi_stt.MPI_TAG;
        /* Get sender's prc_rnk */
        rnk_wrk=wrk_id_bfr[0];
	
        /* Allocate next variable, if any, to worker */
        if(msg_tag_typ == msg_tag_wrk_rqs){
          var_wrt_nbr++; /* [nbr] Number of variables written */
          /* Worker closed output file before sending msg_tag_wrk_rqs */
          TKN_WRT_FREE=True;
	  
          if(idx > nbr_var_prc-1){
            msg_bfr[0]=idx_all_wrk_ass; /* [enm] All variables already assigned */
            msg_bfr[1]=out_id; /* Output file ID */
          }else{
            /* Tell requesting worker to allocate space for next variable */
            msg_bfr[0]=idx; /* [idx] Variable to be processed */
            msg_bfr[1]=out_id; /* Output file ID */
            msg_bfr[2]=var_prc_out[idx]->id; /* [id] Variable ID in output file */
            /* Point to next variable on list */
            idx++;
          } /* endif idx */
          MPI_Send(msg_bfr,msg_bfr_lng,MPI_INT,rnk_wrk,msg_tag_wrk_rsp,MPI_COMM_WORLD);
          /* msg_tag_typ != msg_tag_wrk_rqs */
        }else if(msg_tag_typ == msg_tag_tkn_wrt_rqs){
          /* Allocate token if free, else ask worker to try later */
          if(TKN_WRT_FREE){
            TKN_WRT_FREE=False;
            msg_bfr[0]=tkn_wrt_rqs_xcp; /* Accept request for write token */
          }else{
            msg_bfr[0]=tkn_wrt_rqs_dny; /* Deny request for write token */
          } /* !TKN_WRT_FREE */
          MPI_Send(msg_bfr,msg_bfr_lng,MPI_INT,rnk_wrk,msg_tag_tkn_wrt_rsp,MPI_COMM_WORLD);
        } /* msg_tag_typ != msg_tag_tkn_wrt_rqs */
      } /* end while var_wrt_nbr < nbr_var_prc */
    }else{ /* prc_rnk != rnk_mgr, end Manager code begin Worker code */
      wrk_id_bfr[0]=prc_rnk;
      while(1){ /* While work remains... */
        /* Send msg_tag_wrk_rqs */
        wrk_id_bfr[0]=prc_rnk;
        MPI_Send(wrk_id_bfr,wrk_id_bfr_lng,MPI_INT,rnk_mgr,msg_tag_wrk_rqs,MPI_COMM_WORLD);
        /* Receive msg_tag_wrk_rsp */
        MPI_Recv(msg_bfr,msg_bfr_lng,MPI_INT,0,msg_tag_wrk_rsp,MPI_COMM_WORLD,&mpi_stt);
        idx=msg_bfr[0];
        out_id=msg_bfr[1];
        if(idx == idx_all_wrk_ass) break;
        else{
	  lcl_idx_lst[lcl_nbr_var]=idx; /* storing the indices for subsequent processing by the worker */
          lcl_nbr_var++;
          var_prc_out[idx]->id=msg_bfr[2];
          /* Process this variable same as UP code */
#if 0
	      /* NB: Immediately preceding MPI else scope confounds Emacs indentation
		 Fake end scope restores correct indentation, simplifies code-checking */
	} /* fake end else */
#endif /* !0 */
#else /* !ENABLE_MPI */
#ifdef _OPENMP
#pragma omp parallel for default(none) private(idx,in_id) shared(aed_lst_add_fst,aed_lst_scl_fct,dbg_lvl,dmn_idx_out_in,dmn_rdr_nbr,dmn_rvr_in,fp_stderr,fp_stdout,in_id_arr,nbr_var_prc,nco_pck_map,nco_pck_plc,out_id,prg_nm,rcd,var_prc,var_prc_out)
#endif /* !_OPENMP */
	/* UP and SMP codes main loop over variables */
	for(idx=0;idx<nbr_var_prc;idx++){ /* Process all variables in current file */
#endif /* ENABLE_MPI */
	  in_id=in_id_arr[omp_get_thread_num()];
	  /* fxm TODO nco638 temporary fix? */
	  var_prc[idx]->nc_id=in_id; 
	  if(dbg_lvl > 1) rcd+=nco_var_prc_crr_prn(idx,var_prc[idx]->nm);
	  if(dbg_lvl > 0) (void)fflush(fp_stderr);
	  
	  /* Retrieve variable from disk into memory */
	  /* NB: nco_var_get() with same nc_id contains OpenMP critical region */
	  (void)nco_var_get(in_id,var_prc[idx]);
	  
	  if(dmn_rdr_nbr > 0){
	    if((var_prc_out[idx]->val.vp=(void *)nco_malloc_flg(var_prc_out[idx]->sz*nco_typ_lng(var_prc_out[idx]->type))) == NULL){
	      (void)fprintf(fp_stdout,"%s: ERROR Unable to malloc() %ld*%lu bytes for value buffer for variable %s in main()\n",prg_nm_get(),var_prc_out[idx]->sz,(unsigned long)nco_typ_lng(var_prc_out[idx]->type),var_prc_out[idx]->nm);
	      nco_exit(EXIT_FAILURE); 
	    } /* endif err */
	    
	      /* Change dimensionionality of values */
	    rcd=nco_var_dmn_rdr_val(var_prc[idx],var_prc_out[idx],dmn_idx_out_in[idx],dmn_rvr_in[idx]);
	    /* Re-ordering required two value buffers, time to free input buffer */
	    var_prc[idx]->val.vp=nco_free(var_prc[idx]->val.vp);
	    /* Free current dimension correspondence */
	    dmn_idx_out_in[idx]=nco_free(dmn_idx_out_in[idx]);
	    dmn_rvr_in[idx]=nco_free(dmn_rvr_in[idx]);
	  } /* endif dmn_rdr_nbr > 0 */
	  
	  if(nco_pck_plc != nco_pck_plc_nil){
	    /* Copy input variable buffer to processed variable buffer */
	    /* fxm: this is dangerous and leads to double free()'ing variable buffer */
	    var_prc_out[idx]->val=var_prc[idx]->val;
	    /* (Un-)Pack variable according to packing specification */
	    nco_pck_val(var_prc[idx],var_prc_out[idx],nco_pck_map,nco_pck_plc,aed_lst_add_fst+idx,aed_lst_scl_fct+idx);
	  } /* endif dmn_rdr_nbr > 0 */
	  
#ifdef ENABLE_MPI
	  /* Obtain token and prepare to write */
	  while(1){ /* Send msg_tag_tkn_wrt_rqs repeatedly until token obtained */
	    wrk_id_bfr[0]=prc_rnk;
	    MPI_Send(wrk_id_bfr,wrk_id_bfr_lng,MPI_INT,rnk_mgr,msg_tag_tkn_wrt_rqs,MPI_COMM_WORLD);
	    MPI_Recv(msg_bfr,msg_bfr_lng,MPI_INT,rnk_mgr,msg_tag_tkn_wrt_rsp,MPI_COMM_WORLD,&mpi_stt);
	    tkn_wrt_rsp=msg_bfr[0];
	    /* Wait then re-send request */
	    if(tkn_wrt_rsp == tkn_wrt_rqs_dny) sleep(tkn_wrt_rqs_ntv); else break;
	  } /* end while loop waiting for write token */
	  
	    /* Worker has token---prepare to write */
	  if(tkn_wrt_rsp == tkn_wrt_rqs_xcp){
	    rcd=nco_open(fl_out_tmp,NC_WRITE|NC_SHARE,&out_id);
	    /* Turn off default filling behavior to enhance efficiency */
	    rcd=nco_set_fill(out_id,NC_NOFILL,&fll_md_old);
#else /* !ENABLE_MPI */
#ifdef _OPENMP
#pragma omp critical
#endif /* _OPENMP */
#endif /* !ENABLE_MPI */
	    { /* begin OpenMP critical */
	      /* Common code for UP, SMP, and MPI */
	      /* Copy variable to output file then free value buffer */
	      if(var_prc_out[idx]->nbr_dim == 0){
		(void)nco_put_var1(out_id,var_prc_out[idx]->id,var_prc_out[idx]->srt,var_prc_out[idx]->val.vp,var_prc_out[idx]->type);
	      }else{ /* end if variable is scalar */
		(void)nco_put_vara(out_id,var_prc_out[idx]->id,var_prc_out[idx]->srt,var_prc_out[idx]->cnt,var_prc_out[idx]->val.vp,var_prc_out[idx]->type);
	      } /* end if variable is array */
	    } /* end OpenMP critical */
	      /* Free current output buffer */
	    var_prc_out[idx]->val.vp=nco_free(var_prc_out[idx]->val.vp);
	    
#ifdef ENABLE_MPI
	    /* Close output file and increment written counter */
	    nco_close(out_id);
	    var_wrt_nbr++;
	  } /* endif tkn_wrt_rqs_xcp */
	} /* end else !idx_all_wrk_ass */
      } /* end while loop requesting work/token */
    } /* endif Worker */
#else /* !ENABLE_MPI */
  }  /* end (OpenMP parallel for) loop over idx */
#endif /* !ENABLE_MPI */
  
  if(dbg_lvl > 0) (void)fprintf(fp_stderr,"\n");
  
#ifdef ENABLE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
  if(prc_rnk == rnk_mgr) { /* Manager only */
    MPI_Send(msg_bfr,msg_bfr_lng,MPI_INT,prc_rnk+1,msg_tag_tkn_wrt_rsp,MPI_COMM_WORLD);
  } /* ! prc_rnk == rnk_mgr */
  else{ /* Workers */
    MPI_Recv(msg_bfr,msg_bfr_lng,MPI_INT,prc_rnk-1,msg_tag_tkn_wrt_rsp,MPI_COMM_WORLD,&mpi_stt);
#endif /* !ENABLE_MPI */
    
    /* Write/overwrite packing attributes for newly packed and re-packed variables 
       Logic here should nearly mimic logic in nco_var_dfn() */
    if(nco_pck_plc != nco_pck_plc_nil && nco_pck_plc != nco_pck_plc_upk){
      nco_bool nco_pck_plc_alw; /* [flg] Packing policy allows packing nc_typ_in */
      /* ...put file in define mode to allow metadata writing... */
      rcd=nco_open(fl_out_tmp,NC_WRITE,&out_id);
      (void)nco_redef(out_id);
      /* ...loop through all variables that may have been packed... */
#ifdef ENABLE_MPI
      for(jdx=0;jdx<lcl_nbr_var;jdx++){
	idx=lcl_idx_lst[jdx];
#else /* !ENABLE_MPI */
	for(idx=0;idx<nbr_var_prc;idx++){
#endif /* !ENABLE_MPI */
	  /* nco_var_dfn() pre-defined dummy packing attributes in output file 
	     only for input variables considered "packable" */
	  if((nco_pck_plc_alw=nco_pck_plc_typ_get(nco_pck_map,var_prc[idx]->typ_upk,(nc_type *)NULL))){
	    /* Verify input variable was newly packed by this operator
	       Writing pre-existing (non-re-packed) attributes here would fail because
	       nco_pck_dsk_inq() never fills in var->scl_fct.vp and var->add_fst.vp
	       Logic is same as in nco_var_dfn() (except var_prc[] instead of var[])
	       If operator newly packed this particular variable... */
	    if(
	       /* ...either because operator newly packs all variables... */
	       (nco_pck_plc == nco_pck_plc_all_new_att) ||
	       /* ...or because operator newly packs un-packed variables like this one... */
	       (nco_pck_plc == nco_pck_plc_all_xst_att && !var_prc[idx]->pck_ram) ||
	       /* ...or because operator re-packs packed variables like this one... */
	       (nco_pck_plc == nco_pck_plc_xst_new_att && var_prc[idx]->pck_ram)
	       ){
	      /* Replace dummy packing attributes with final values, or delete them */
	      if(dbg_lvl >= 5) (void)fprintf(stderr,"%s: main() replacing dummy packing attribute values for variable %s\n",prg_nm,var_prc[idx]->nm);
	      (void)nco_aed_prc(out_id,aed_lst_add_fst[idx].id,aed_lst_add_fst[idx]);
	      (void)nco_aed_prc(out_id,aed_lst_scl_fct[idx].id,aed_lst_scl_fct[idx]);
	    } /* endif variable is newly packed by this operator */
	  } /* endif nco_pck_plc_alw */
	} /* end loop over var_prc */
	(void)nco_enddef(out_id);
#ifdef ENABLE_MPI
	nco_close(out_id);
#endif /* !ENABLE_MPI */
      } /* nco_pck_plc == nco_pck_plc_nil || nco_pck_plc == nco_pck_plc_upk */
      
#ifdef ENABLE_MPI
      if(prc_rnk == prc_nbr-1) /* Send Token to Manager */
	MPI_Send(msg_bfr,msg_bfr_lng,MPI_INT,rnk_mgr,msg_tag_tkn_wrt_rsp,MPI_COMM_WORLD);
      else
	MPI_Send(msg_bfr,msg_bfr_lng,MPI_INT,prc_rnk+1,msg_tag_tkn_wrt_rsp,MPI_COMM_WORLD);
    } /* !Workers */
    if(prc_rnk == rnk_mgr){ /* Only Manager */
      MPI_Recv(msg_bfr,msg_bfr_lng,MPI_INT,prc_nbr-1,msg_tag_tkn_wrt_rsp,MPI_COMM_WORLD,&mpi_stt);
    } /* prc_rnk != rnk_mgr */
#endif /* !ENABLE_MPI */
    
    /* Close input netCDF file */
    for(thr_idx=0;thr_idx<thr_nbr;thr_idx++) nco_close(in_id_arr[thr_idx]);
    
    /* Remove local copy of file */
    if(FILE_RETRIEVED_FROM_REMOTE_LOCATION && REMOVE_REMOTE_FILES_AFTER_PROCESSING) (void)nco_fl_rm(fl_in);
    
  } /* end loop over fl_idx */
  
#ifdef ENABLE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
  /* Manager moves output file (closed by workers) from temporary to permanent location */
  if(prc_rnk == rnk_mgr) (void)nco_fl_mv(fl_out_tmp,fl_out);
#else /* !ENABLE_MPI */
  /* Close output file and move it from temporary to permanent location */
  (void)nco_fl_out_cls(fl_out,fl_out_tmp,out_id);
#endif /* end !ENABLE_MPI */
  
  /* ncpdq-specific memory cleanup */
  if(dmn_rdr_nbr > 0){
    /* Free dimension correspondence list */
    for(idx=0;idx<nbr_var_prc;idx++){
      dmn_idx_out_in[idx]=(int *)nco_free(dmn_idx_out_in[idx]);
      dmn_rvr_in[idx]=(nco_bool *)nco_free(dmn_rvr_in[idx]);
    } /* end loop over idx */
    if(dmn_idx_out_in != NULL) dmn_idx_out_in=(int **)nco_free(dmn_idx_out_in);
    if(dmn_rvr_in != NULL) dmn_rvr_in=(nco_bool **)nco_free(dmn_rvr_in);
    if(dmn_rvr_rdr != NULL) dmn_rvr_rdr=(nco_bool *)nco_free(dmn_rvr_rdr);
    if(dmn_rdr_nbr_in > 0) dmn_rdr_lst_in=nco_sng_lst_free(dmn_rdr_lst_in,dmn_rdr_nbr_in);
    /* Free dimension list pointers */
    dmn_rdr=(dmn_sct **)nco_free(dmn_rdr);
    /* Dimension structures in dmn_rdr are owned by dmn and dmn_out, free'd later */
  } /* endif dmn_rdr_nbr > 0 */
  if(nco_pck_plc != nco_pck_plc_nil){
    if(nco_pck_plc_sng != NULL) nco_pck_plc_sng=(char *)nco_free(nco_pck_plc_sng);
    if(nco_pck_map_sng != NULL) nco_pck_map_sng=(char *)nco_free(nco_pck_map_sng);
    if(nco_pck_plc != nco_pck_plc_upk){
      /* No need for loop over var_prc variables to free attribute values
	 Variable structures and attribute edit lists share same attribute values
	 Free them only once, and do it in nco_var_free() */
      aed_lst_add_fst=(aed_sct *)nco_free(aed_lst_add_fst);
      aed_lst_scl_fct=(aed_sct *)nco_free(aed_lst_scl_fct);
    } /* nco_pck_plc == nco_pck_plc_upk */
  } /* nco_pck_plc == nco_pck_plc_nil */
  
    /* NCO-generic clean-up */
    /* Free individual strings/arrays */
  if(cmd_ln != NULL) cmd_ln=(char *)nco_free(cmd_ln);
  if(fl_in != NULL) fl_in=(char *)nco_free(fl_in);
  if(fl_out != NULL) fl_out=(char *)nco_free(fl_out);
  if(fl_out_tmp != NULL) fl_out_tmp=(char *)nco_free(fl_out_tmp);
  if(fl_pth != NULL) fl_pth=(char *)nco_free(fl_pth);
  if(fl_pth_lcl != NULL) fl_pth_lcl=(char *)nco_free(fl_pth_lcl);
  if(in_id_arr != NULL) in_id_arr=(int *)nco_free(in_id_arr);
  /* Free lists of strings */
  if(fl_lst_in != NULL && fl_lst_abb == NULL) fl_lst_in=nco_sng_lst_free(fl_lst_in,fl_nbr); 
  if(fl_lst_in != NULL && fl_lst_abb != NULL) fl_lst_in=nco_sng_lst_free(fl_lst_in,1);
  if(fl_lst_abb != NULL) fl_lst_abb=nco_sng_lst_free(fl_lst_abb,abb_arg_nbr);
  if(var_lst_in_nbr > 0) var_lst_in=nco_sng_lst_free(var_lst_in,var_lst_in_nbr);
  /* Free limits */
  for(idx=0;idx<lmt_nbr;idx++) lmt_arg[idx]=(char *)nco_free(lmt_arg[idx]);
  if(lmt_nbr > 0) lmt=nco_lmt_lst_free(lmt,lmt_nbr);
  /* Free dimension lists */
  if(nbr_dmn_xtr > 0) dim=nco_dmn_lst_free(dim,nbr_dmn_xtr);
  if(nbr_dmn_xtr > 0) dmn_out=nco_dmn_lst_free(dmn_out,nbr_dmn_xtr);
  /* Free variable lists */
  if(nbr_xtr > 0) var=nco_var_lst_free(var,nbr_xtr);
  if(nbr_xtr > 0) var_out=nco_var_lst_free(var_out,nbr_xtr);
  var_prc=(var_sct **)nco_free(var_prc);
  var_prc_out=(var_sct **)nco_free(var_prc_out);
  var_fix=(var_sct **)nco_free(var_fix);
  var_fix_out=(var_sct **)nco_free(var_fix_out);
  
#ifdef ENABLE_MPI
  MPI_Finalize();
#endif /* !ENABLE_MPI */
  
  nco_exit_gracefully();
  return EXIT_SUCCESS;
} /* end main() */

