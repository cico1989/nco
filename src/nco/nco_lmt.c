/* $Header: /data/zender/nco_20150216/nco/src/nco/nco_lmt.c,v 1.84 2009-08-17 16:21:21 hmb Exp $ */

/* Purpose: Hyperslab limits */

/* Copyright (C) 1995--2009 Charlie Zender
   License: GNU General Public License (GPL) Version 3
   See http://www.gnu.org/copyleft/gpl.html for full license text */

#include "nco_lmt.h" /* Hyperslab limits */

lmt_sct * /* O [sct] Pointer to free'd structure */
nco_lmt_free /* [fnc] Free memory associated with limit structure */
(lmt_sct *lmt) /* I/O [sct] Limit structure to free */
{
  /* Threads: Routine is thread safe and calls no unsafe routines */
  /* Purpose: Free all memory associated with dynamically allocated limit structure */
  lmt->nm=(char *)nco_free(lmt->nm);
  lmt->min_sng=(char *)nco_free(lmt->min_sng);
  lmt->max_sng=(char *)nco_free(lmt->max_sng);
  lmt->srd_sng=(char *)nco_free(lmt->srd_sng);
  lmt->re_bs_sng=(char*)nco_free(lmt->re_bs_sng);   

  lmt=(lmt_sct *)nco_free(lmt);

  return lmt;
} /* end nco_lmt_free() */

lmt_sct ** /* O [sct] Pointer to free'd structure list */
nco_lmt_lst_free /* [fnc] Free memory associated with limit structure list */
(lmt_sct **lmt_lst, /* I/O [sct] Limit structure list to free */
 const int lmt_nbr) /* I [nbr] Number of limit structures in list */
{
  /* Threads: Routine is thread safe and calls no unsafe routines */
  /* Purpose: Free all memory associated with dynamically allocated limit structure list */
  int idx;

  for(idx=0;idx<lmt_nbr;idx++) lmt_lst[idx]=nco_lmt_free(lmt_lst[idx]);

  /* Free structure pointer last */
  lmt_lst=(lmt_sct **)nco_free(lmt_lst);

  return lmt_lst;
} /* end nco_lmt_lst_free() */

lmt_all_sct * /* O [sct] Pointer to free'd structure */
nco_lmt_all_free /* [fnc] Free memory associated with limit structure */
(lmt_all_sct *lmt_all) /* I/O [sct] Limit structure to free */
{
  /* Threads: Routine is thread safe and calls no unsafe routines */
  /* Purpose: Free all memory associated with dynamically allocated lmt_all structure */
  lmt_all->dmn_nm=(char *)nco_free(lmt_all->dmn_nm);
  /* NB: lmt_dmn[idx] are free'd by nco_lmt_lst_free() in calling routine */
  lmt_all->lmt_dmn=(lmt_sct **)nco_free(lmt_all->lmt_dmn);

  lmt_all=(lmt_all_sct *)nco_free(lmt_all);

  return lmt_all;
} /* end nco_lmt_all_free() */

lmt_all_sct ** /* O [sct] Pointer to free'd structure list */
nco_lmt_all_lst_free /* [fnc] Free memory associated with lmt_all structure list */
(lmt_all_sct **lmt_all_lst, /* I/O [sct] Limit structure list to free */
 const int lmt_all_nbr) /* I [nbr] Number of limit strucgtures in list */
{
  /* Threads: Routine is thread safe and calls no unsafe routines */
  /* Purpose: Free all memory associated with dynamically allocated limit structure list */
  int idx;

  for(idx=0;idx<lmt_all_nbr;idx++) lmt_all_lst[idx]=nco_lmt_all_free(lmt_all_lst[idx]);

  /* Free structure pointer last */
  lmt_all_lst=(lmt_all_sct **)nco_free(lmt_all_lst);

  return lmt_all_lst;
} /* end nco_lmt_all_lst_free() */

lmt_sct * /* [sct] Limit structure for dimension */
nco_lmt_sct_mk /* [fnc] Create stand-alone limit structure for given dimension */
(const int nc_id, /* I [idx] netCDF file ID */
 const int dmn_id, /* I [idx] ID of dimension for this limit structure */
 CST_X_PTR_CST_PTR_CST_Y(lmt_sct,lmt), /* I [sct] Array of limit structures from nco_lmt_evl() */ 
 int lmt_nbr, /* I [nbr] Number of limit structures */
 const nco_bool FORTRAN_IDX_CNV) /* I [flg] Hyperslab indices obey Fortran convention */
{
  /* Purpose: Create stand-alone limit structure just for given dimension
     ncra calls nco_lmt_sct_mk() to generate record dimension limit structure */
  
  int idx;
  int rcd; /* [rcd] Return code */
  
  lmt_sct *lmt_dim;
  
  lmt_dim=(lmt_sct *)nco_malloc(sizeof(lmt_sct));

  /* Initialize defaults to False, override later if warranted */
  lmt_dim->is_usr_spc_lmt=False; /* True if any part of limit is user-specified, else False */
  lmt_dim->is_usr_spc_max=False; /* True if user-specified, else False */
  lmt_dim->is_usr_spc_min=False; /* True if user-specified, else False */
  /* rec_skp_nsh_spf is used for record dimension in multi-file operators */
  lmt_dim->rec_skp_nsh_spf=0L; /* Number of records skipped in initial superfluous files */
  
  for(idx=0;idx<lmt_nbr;idx++){
    /* Copy user-specified limits, if any */
    if(lmt[idx]->id == dmn_id){
      lmt_dim->is_usr_spc_lmt=True; /* True if any part of limit is user-specified, else False */
      if(lmt[idx]->max_sng == NULL){
	lmt_dim->max_sng=NULL;
      }else{
	lmt_dim->max_sng=(char *)strdup(lmt[idx]->max_sng);
	lmt_dim->is_usr_spc_max=True; /* True if user-specified, else False */
      } /* end if */
      if(lmt[idx]->min_sng == NULL){
	lmt_dim->min_sng=NULL;
      }else{
	lmt_dim->min_sng=(char *)strdup(lmt[idx]->min_sng);
	lmt_dim->is_usr_spc_min=True; /* True if user-specified, else False */
      } /* end if */
      if(lmt[idx]->srd_sng != NULL) lmt_dim->srd_sng=(char *)strdup(lmt[idx]->srd_sng); else lmt_dim->srd_sng=NULL;
      lmt_dim->nm=(char *)strdup(lmt[idx]->nm);
      break;
    } /* end if */
  } /* end loop over idx */
  
  /* If this limit was not user-specified, then ... */
  if(idx == lmt_nbr){
    /* Create default limits to look as though user-specified them */
    char dmn_nm[NC_MAX_NAME];
    long cnt;
    int max_sng_sz;
    
    /* Fill in limits with default parsing information */
    rcd=nco_inq_dim_flg(nc_id,dmn_id,dmn_nm,&cnt);
    
    if(rcd == NC_EBADDIM){
      (void)fprintf(stdout,"%s: ERROR attempting to find non-existent dimension with id = %d in nco_lmt_sct_mk()\n",prg_nm_get(),dmn_id);
      nco_exit(EXIT_FAILURE);
    } /* end if */
    
    lmt_dim->nm=(char *)strdup(dmn_nm);
    lmt_dim->srd_sng=NULL;
    /* Generate min and max strings to look as if user had specified them
       Adjust accordingly if FORTRAN_IDX_CNV was requested for other dimensions
       These sizes will later be decremented in nco_lmt_evl() where all information
       is converted internally to C based indexing representation.
       Ultimately this problem arises because I want nco_lmt_evl() to think the
       user always did specify this dimension's hyperslab.
       Otherwise, problems arise when FORTRAN_IDX_CNV is specified by the user 
       along with explicit hypersalbs for some dimensions excluding the record
       dimension.
       Then, when nco_lmt_sct_mk() creates the record dimension structure, it must
       be created consistently with the FORTRAN_IDX_CNV flag for the other dimensions.
       In order to do that, fill in max_sng, min_sng, and srd_sng
       arguments with strings as if they had been read from keyboard.
       An alternate solution is to add flag to lmt_sct indicating whether this
       limit struct had been automatically generated and then act accordingly. */
    /* Decrement cnt to C index value if necessary */
    if(!FORTRAN_IDX_CNV) cnt--; 
    if(cnt < 0L){
      if(cnt == -1L) (void)fprintf(stdout,"%s: ERROR nco_lmt_sct_mk() reports record variable exists and is size zero, i.e., has no records yet.\n",prg_nm_get());
      (void)fprintf(stdout,"%s: HINT: Perform record-oriented operations only after file has valid records.\n",prg_nm_get());
      (void)fprintf(stdout,"%s: cnt < 0 in nco_lmt_sct_mk()\n",prg_nm_get());
      nco_exit(EXIT_FAILURE);
    } /* end if */
    /* cnt < 10 covers negative numbers and SIGFPE from log10(cnt==0) 
       Adding 1 is required for cnt=10,100,1000... */
    if(cnt < 10L) max_sng_sz=1; else max_sng_sz=1+(int)ceil(log10((double)cnt));
    /* Add one for NUL terminator */
    lmt_dim->max_sng=(char *)nco_malloc(sizeof(char)*(max_sng_sz+1));
    (void)sprintf(lmt_dim->max_sng,"%ld",cnt);
    if(FORTRAN_IDX_CNV){
      lmt_dim->min_sng=(char *)strdup("1");
    }else{
      lmt_dim->min_sng=(char *)strdup("0");
    } /* end else */
  } /* end if user did not explicity specify limits for this dimension */
  
  return lmt_dim;
  
} /* end nco_lmt_sct_mk() */

void
nco_lmt_evl /* [fnc] Parse user-specified limits into hyperslab specifications */
(int nc_id, /* I [idx] netCDF file ID */
 lmt_sct *lmt_ptr, /* I/O [sct] Structure from nco_lmt_prs() or from nco_lmt_sct_mk() to hold dimension limit information */
 long cnt_crr, /* I [nbr] Number of valid records already processed (only used for record dimensions in multi-file operators) */
 nco_bool FORTRAN_IDX_CNV) /* I [flg] Hyperslab indices obey Fortran convention */
{
  /* NB: nco_lmt_evl() with same nc_id contains OpenMP critical region */
  /* Purpose: Take parsed list of dimension names, minima, and
     maxima strings and find appropriate indices into dimensions 
     for formulation of dimension start and count vectors, or fail trying. */
  
  nco_bool flg_no_data=False; /* True if file contains no data for hyperslab */
  nco_bool rec_dmn_and_mlt_fl_opr=False; /* True if record dimension in multi-file operator */
  
  dmn_sct dim;
  
  enum monotonic_direction{
    decreasing, /* 0 */
    increasing}; /* 1 */
  
  lmt_sct lmt;
  
  int min_lmt_typ=int_CEWI;
  int max_lmt_typ=int_CEWI;
  int monotonic_direction;
  int prg_id; /* Program ID */
  int rcd=NC_NOERR; /* [enm] Return code */
  int rec_dmn_id; /* [idx] Variable ID of record dimension, if any */
  
  long idx;
  long dmn_sz;
  long cnt_rmn_crr=-1L; /* Records to extract from current file */
  long cnt_rmn_ttl=-1L; /* Total records remaining to be read from this and all remaining files */
  long rec_skp_vld_prv_dgn=-1L; /* Records skipped at end of previous valid file (diagnostic only) */
    
  char *fl_udu_sng=NULL_CEWI;   /* store units attribute of co-ordinate dim */
          
  lmt=*lmt_ptr;
  
  prg_id=prg_get(); /* Program ID */
  
  /* Initialize limit structure */
  lmt.srd=1L;
  lmt.min_val=0.0;
  lmt.max_val=0.0;
  
  /* Get dimension ID */
  rcd=nco_inq_dimid_flg(nc_id,lmt.nm,&lmt.id);
  if(rcd != NC_NOERR){
    (void)fprintf(stdout,"%s: ERROR dimension %s is not in input file\n",prg_nm_get(),lmt.nm);
    nco_exit(EXIT_FAILURE);
  } /* endif */
  
  /* Logic on whether to allow skipping current file depends on whether limit
     is specified for record dimension in multi-file operators.
     This information is not used in single-file operators, but whether
     the limit is a record limit may be tested.
     Best to program defensively and define this flag in all cases. */
  (void)nco_inq(nc_id,(int *)NULL,(int *)NULL,(int *)NULL,&rec_dmn_id);
  if(lmt.id == rec_dmn_id) lmt.is_rec_dmn=True; else lmt.is_rec_dmn=False;
  if(lmt.is_rec_dmn && (prg_id == ncra || prg_id == ncrcat)) rec_dmn_and_mlt_fl_opr=True; else rec_dmn_and_mlt_fl_opr=False;
  
  /* Get dimension size */
  (void)nco_inq_dimlen(nc_id,lmt.id,&dim.sz);
  
  /* Shortcut to avoid indirection */
  dmn_sz=dim.sz;
  
  /* Bomb if dmn_sz < 1 */
  if(dmn_sz < 1){
    (void)fprintf(stdout,"%s: ERROR Size of dimension \"%s\" is %li in input file, but must be > 0 in order to apply limits.\n",prg_nm_get(),lmt.nm,dmn_sz);
    nco_exit(EXIT_FAILURE);
  } /* end if */
  
  if(lmt.srd_sng != NULL){
    if(strchr(lmt.srd_sng,'.') || strchr(lmt.srd_sng,'e') || strchr(lmt.srd_sng,'E') || strchr(lmt.srd_sng,'d') || strchr(lmt.srd_sng,'D')){
      (void)fprintf(stdout,"%s: ERROR Requested stride for \"%s\", %s, must be integer\n",prg_nm_get(),lmt.nm,lmt.srd_sng);
      nco_exit(EXIT_FAILURE);
    } /* end if */
    lmt.srd=strtol(lmt.srd_sng,(char **)NULL,10);
    if(lmt.srd < 1){
      (void)fprintf(stdout,"%s: ERROR Stride for \"%s\" is %li but must be > 0\n",prg_nm_get(),lmt.nm,lmt.srd);
      nco_exit(EXIT_FAILURE);
    } /* end if */
  } /* end if */
  
  /* If min_sng and max_sng are both NULL then set type to lmt_dmn_idx */
  if(lmt.min_sng == NULL && lmt.max_sng == NULL){
    /* Limiting indices will be set to default extrema a bit later */
    min_lmt_typ=max_lmt_typ=lmt_dmn_idx;
  }else{
    /* min_sng and max_sng are not both NULL */
    /* Limit is coordinate value if string contains decimal point or is in exponential format 
       Otherwise limit is interpreted as zero-based dimension offset */
    if(lmt.min_sng != NULL) min_lmt_typ=nco_lmt_typ(lmt.min_sng);
    if(lmt.max_sng != NULL) max_lmt_typ=nco_lmt_typ(lmt.max_sng);
    
    /* Copy lmt_typ from defined limit to undefined */
    if(lmt.min_sng == NULL) min_lmt_typ=max_lmt_typ;
    if(lmt.max_sng == NULL) max_lmt_typ=min_lmt_typ;
  } /* end else */
  
  /* Both min_lmt_typ and max_lmt_typ are now defined
     Continue only if both limits are of the same type */
  if(min_lmt_typ != max_lmt_typ){
    (void)fprintf(stdout,"%s: ERROR -d %s,%s,%s\n",prg_nm_get(),lmt.nm,lmt.min_sng,lmt.max_sng);
    (void)fprintf(stdout,"Limits on dimension \"%s\" must be of same numeric type:\n",lmt.nm);
    (void)fprintf(stdout,"\"%s\" was interpreted as a %s.\n",lmt.min_sng,(min_lmt_typ == lmt_crd_val) ? "coordinate value" : "zero-based dimension index");
    (void)fprintf(stdout,"\"%s\" was interpreted as a %s.\n",lmt.max_sng,(max_lmt_typ == lmt_crd_val) ? "coordinate value" : "zero-based dimension index");
    (void)fprintf(stdout,"(Limit arguments containing a decimal point are interpreted as coordinate values; arguments without a decimal point are interpreted as zero-based dimensional indices.)\n");
    nco_exit(EXIT_FAILURE);
  } /* end if */
  lmt.lmt_typ=min_lmt_typ;

  /* co-ordinate rebasing code */
  lmt.origin=0.0;
  /* Get variable ID of coordinate */
  rcd=nco_inq_varid_flg(nc_id,lmt.nm,&dim.cid);
  if(rcd==NC_NOERR)
    fl_udu_sng=nco_lmt_get_udu_att(nc_id,dim.cid); /* units attribute of co-ordinate var */

  if(rec_dmn_and_mlt_fl_opr && fl_udu_sng && lmt.re_bs_sng){ 
#ifdef ENABLE_UDUNITS
      /* Re-base and reset origin to 0.0 if re-basing fails */
      if(nco_lmt_clc_org(fl_udu_sng,lmt.re_bs_sng,&lmt.origin)!=EXIT_SUCCESS) lmt.origin=0.0;
#endif /* !ENABLE_UDUNITS */
    } /* endif */

  if((lmt.lmt_typ == lmt_crd_val) || (lmt.lmt_typ == lmt_udu_sng)){
    double *dmn_val_dp;
    
    double dmn_max;
    double dmn_min;
    
    long max_idx;
    long min_idx;
    long tmp_idx;
    long dmn_srt=0L;


    /* Get variable ID of coordinate 
       rcd=nco_inq_varid(nc_id,lmt.nm,&dim.cid); */
    
    /* Get coordinate type */
    (void)nco_inq_vartype(nc_id,dim.cid,&dim.type);
    
    /* Warn when coordinate type is weird */
    if(dim.type == NC_BYTE || dim.type == NC_UBYTE || dim.type == NC_CHAR || dim.type == NC_STRING) (void)fprintf(stderr,"\n%s: WARNING Coordinate %s is type %s. Dimension truncation is unpredictable.\n",prg_nm_get(),lmt.nm,nco_typ_sng(dim.type));
    
    /* Allocate enough space to hold coordinate */
    dim.val.vp=(void *)nco_malloc(dmn_sz*nco_typ_lng(dim.type));
    
#ifdef _OPENMP
#pragma omp critical
#endif /* _OPENMP */
    { /* begin OpenMP critical */
      /* Block is critical for identical in_id's
	 Block is thread-safe for distinct in_id's */
      /* Retrieve this coordinate */
      nco_get_vara(nc_id,dim.cid,&dmn_srt,&dmn_sz,dim.val.vp,dim.type);
    } /* end OpenMP critical */
    
    /* Convert coordinate to double-precision if neccessary */
    if(dim.type != NC_DOUBLE){
      ptr_unn old_val;
      
      old_val=dim.val;
      dim.val.vp=(void *)nco_malloc(dmn_sz*nco_typ_lng(NC_DOUBLE));
      /* Typecast old coordinate pointer union to correct type before access */
      (void)cast_void_nctype(dim.type,&old_val);
      
      /* Shortcut to avoid indirection */
      dmn_val_dp=dim.val.dp;
      switch(dim.type){
      case NC_FLOAT: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.fp[idx];} break; 
      case NC_DOUBLE: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.dp[idx];} break; 
      case NC_INT: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.lp[idx];} break;
      case NC_SHORT: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.sp[idx];} break;
      case NC_USHORT: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.usp[idx];} break;
      case NC_UINT: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.uip[idx];} break;
      case NC_INT64: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.i64p[idx];} break;
      case NC_UINT64: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.ui64p[idx];} break;
      case NC_BYTE: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.bp[idx];} break;
      case NC_UBYTE: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.ubp[idx];} break;
      case NC_CHAR: for(idx=0L;idx<dmn_sz;idx++) {dmn_val_dp[idx]=old_val.cp[idx];} break;
      case NC_STRING: break; /* Do nothing */
      default: nco_dfl_case_nc_type_err(); break;
      } /* end switch */
      
      /* Un-typecast pointer to values after access */
      (void)cast_nctype_void(dim.type,&old_val);
      
      /* Free original space allocated for dimension */
      old_val.vp=nco_free(old_val.vp);
      
      /* Officially change type */
      dim.type=NC_DOUBLE;
    } /* end type conversion */
    
    /* Shortcut to avoid indirection */
    dmn_val_dp=dim.val.dp;
    
    /* Assuming coordinate is monotonic, direction of monotonicity is determined by first two elements */
    if(dmn_sz == 1){
      monotonic_direction=increasing;
    }else{
      if(dmn_val_dp[0] > dmn_val_dp[1]) monotonic_direction=decreasing; else monotonic_direction=increasing;
    } /* end else */
    
    if(monotonic_direction == increasing){
      min_idx=0L;
      max_idx=dmn_sz-1L;
    }else{
      min_idx=dmn_sz-1L;
      max_idx=0L;
    } /* end else */
    
    /* Determine min and max values of entire coordinate */
    dmn_min=dmn_val_dp[min_idx];
    dmn_max=dmn_val_dp[max_idx];
    
    /* Convert UDUnits strings if necessary */
    if(lmt.lmt_typ == lmt_udu_sng){
      
      if(!fl_udu_sng){ 
        (void)fprintf(stdout,"%s: ERROR attempting to read units attribute from variable \"%s\" \n",prg_nm_get(),dim.nm);
        nco_exit(EXIT_FAILURE);
      } /* end if */
      
      if(nco_lmt_udu_cnv(dim.cid,fl_udu_sng,lmt.min_sng,&lmt.min_val)) nco_exit(EXIT_FAILURE);
      if(nco_lmt_udu_cnv(dim.cid,fl_udu_sng,lmt.max_sng,&lmt.max_val)) nco_exit(EXIT_FAILURE);
 
    }else{ /* end UDUnits conversion */
      /* Convert user-specified limits into double precision numeric values, or supply defaults */
      if(lmt.min_sng == NULL) lmt.min_val=dmn_val_dp[min_idx]; else lmt.min_val=strtod(lmt.min_sng,(char **)NULL);
      if(lmt.max_sng == NULL) lmt.max_val=dmn_val_dp[max_idx]; else lmt.max_val=strtod(lmt.max_sng,(char **)NULL);
      
      /* re-base co-ordinates as necessary in multi-file operatators 
         lmt.origin has been calculated earlier in file */
      if(rec_dmn_and_mlt_fl_opr){ 
        lmt.min_val-=lmt.origin;
        lmt.max_val-=lmt.origin;     
      }
    }
    /* Warn when min_val > max_val (i.e., wrapped coordinate)*/
    if(dbg_lvl_get() > nco_dbg_std && lmt.min_val > lmt.max_val) (void)fprintf(stderr,"%s: INFO Interpreting hyperslab specifications as wrapped coordinates [%s <= %g] and [%s >= %g]\n",prg_nm_get(),lmt.nm,lmt.max_val,lmt.nm,lmt.min_val);
    
    /* Fail when... */
    if(
       /* Following condition added 20000508, changes behavior of single point 
	  hyperslabs depending on whether hyperslab occurs in record dimension
	  during multi-file operator operation.
	  Altered behavior of single point hyperslabs so that single point
	  hyperslabs in the record coordinate (i.e., -d time,1.0,1.0) may be
	  treated differently than single point hyperslabs in other
	  coordinates. Multifile operators will skip files if single point
	  hyperslabs in record coordinate lays outside record coordinate
	  range of file. For non-record coordinates (and for all operators
	  besides ncra and ncrcat on record coordinates), single point
	  hyperslabs will choose the closest value rather than skip the file
	  (I believe). This should be verified. */
       /* User specified single point, coordinate is not wrapped, and both extrema fall outside valid crd range */
       (rec_dmn_and_mlt_fl_opr && (lmt.min_val == lmt.max_val) && ((lmt.min_val > dmn_max) || (lmt.max_val < dmn_min))) ||
       /* User did not specify single point, coordinate is not wrapped, and either extrema falls outside valid crd range */
       ((lmt.min_val < lmt.max_val) && ((lmt.min_val > dmn_max) || (lmt.max_val < dmn_min))) ||
       /* User did not specify single point, coordinate is wrapped, and both extrema fall outside valid crd range */
       ((lmt.min_val > lmt.max_val) && ((lmt.min_val > dmn_max) && (lmt.max_val < dmn_min))) ||
       False){
      /* Allow for possibility that current file is superfluous */
      if(rec_dmn_and_mlt_fl_opr){
	flg_no_data=True;
	goto no_data;
      }else{
	(void)fprintf(stdout,"%s: ERROR User-specified coordinate value range %g <= %s <= %g does not fall within valid coordinate range %g <= %s <= %g\n",prg_nm_get(),lmt.min_val,lmt.nm,lmt.max_val,dmn_min,lmt.nm,dmn_max);
	nco_exit(EXIT_FAILURE);
      } /* end else */
    } /* end if */
    
    /* Armed with target coordinate minima and maxima, we are ready to bracket user-specified range */
    
    /* If min_sng or max_sng were omitted, use extrema */
    if(lmt.min_sng == NULL) lmt.min_idx=min_idx;
    if(lmt.max_sng == NULL) lmt.max_idx=max_idx;
    
    /* Single slice requires finding the closest coordinate */
    if(lmt.min_val == lmt.max_val){
      double dst_new;
      double dst_old;
      
      lmt.min_idx=0L;
      dst_old=fabs(lmt.min_val-dmn_val_dp[0]);
      for(tmp_idx=1L;tmp_idx<dmn_sz;tmp_idx++){
	if((dst_new=fabs(lmt.min_val-dmn_val_dp[tmp_idx])) < dst_old){
	  dst_old=dst_new;
	  lmt.min_idx=tmp_idx;
	} /* end if */
      } /* end loop over tmp_idx */
      lmt.max_idx=lmt.min_idx;
      
    }else{ /* min_val != max_val */
      
      /* Find brackets to specified extrema */
      /* Should no coordinate values match the given criteria, flag that index with -1L
	 We defined the valid syntax such that single half range with -1L is not an error
	 This causes "-d lon,100.0,-100.0" to select [-180.0] when lon=[-180.0,-90.0,0.0,90.0] because one
	 of the specified half-ranges is valid (there are coordinates < -100.0).
	 However, "-d lon,100.0,-200.0" should fail when lon=[-180.0,-90.0,0.0,90.0] because both 
	 of the specified half-ranges are invalid (no coordinate is > 100.0 or < -200.0).
	 The -1L flags are replaced with the correct indices (0L or dmn_sz-1L) following the search loop block
	 Overwriting the -1L flags with 0L or dmn_sz-1L later is more heuristic than setting them = 0L here,
	 since 0L is valid search result.
      */
      if(monotonic_direction == increasing){
	if(lmt.min_sng != NULL){
	  /* Find index of smallest coordinate greater than min_val */
	  tmp_idx=0L;
	  while((dmn_val_dp[tmp_idx] < lmt.min_val) && (tmp_idx < dmn_sz)) tmp_idx++;
	  if(tmp_idx != dmn_sz) lmt.min_idx=tmp_idx; else lmt.min_idx=-1L;
	} /* end if */
	if(lmt.max_sng != NULL){
	  /* Find index of largest coordinate less than max_val */
	  tmp_idx=dmn_sz-1L;
	  while((dmn_val_dp[tmp_idx] > lmt.max_val) && (tmp_idx > -1L)) tmp_idx--;
	  if(tmp_idx != -1L) lmt.max_idx=tmp_idx; else lmt.max_idx=-1L;
	} /* end if */
	/* end if monotonic_direction == increasing */
      }else{ /* monotonic_direction == decreasing */
	if(lmt.min_sng != NULL){
	  /* Find index of smallest coordinate greater than min_val */
	  tmp_idx=dmn_sz-1L;
	  while((dmn_val_dp[tmp_idx] < lmt.min_val) && (tmp_idx > -1L)) tmp_idx--;
	  if(tmp_idx != -1L) lmt.min_idx=tmp_idx; else lmt.min_idx=-1L;
	} /* end if */
	if(lmt.max_sng != NULL){
	  /* Find index of largest coordinate less than max_val */
	  tmp_idx=0L;
	  while((dmn_val_dp[tmp_idx] > lmt.max_val) && (tmp_idx < dmn_sz)) tmp_idx++;
	  if(tmp_idx != dmn_sz) lmt.max_idx=tmp_idx; else lmt.max_idx=-1L;
	} /* end if */
      } /* end else monotonic_direction == decreasing */
      
      /* Case where both min_idx and max_idx = -1 was flagged as error above
	 Case of wrapped coordinate: Either, but not both, of min_idx or max_idx will be flagged with -1
	 See explanation above */
      if(lmt.min_idx == -1L && (lmt.min_val > lmt.max_val)) lmt.min_idx=0L;
      if(lmt.max_idx == -1L && (lmt.min_val > lmt.max_val)) lmt.max_idx=dmn_sz-1L;
      
    } /* end if min_val != max_val */
    
    /* User-specified ranges are now bracketed */
    
    /* Convert indices of minima and maxima to srt and end indices */
    if(monotonic_direction == increasing){
      lmt.srt=lmt.min_idx;
      lmt.end=lmt.max_idx;
    }else{
      lmt.srt=lmt.max_idx;
      lmt.end=lmt.min_idx;
    }  /* end else */
    
    /* Un-typecast pointer to values after access */
    (void)cast_nctype_void((nc_type)NC_DOUBLE,&dim.val);
  
    /* Free space allocated for dimension */
    dim.val.vp=nco_free(dim.val.vp);
    
  }else{ /* end if limit arguments were coordinate values */
    /* Convert limit strings to zero-based indicial offsets */
    
    /* Specifying stride alone, but not min or max, is legal, e.g., -d time,,,2
       Thus is_usr_spc_lmt may be True, even though one or both of min_sng, max_sng is NULL
       Furthermore, both min_sng and max_sng are artifically created by nco_lmt_sct_mk()
       for record dimensions when the user does not explicitly specify limits.
       In this case, min_sng_and max_sng are non-NULL though no limits were specified
       In fact, min_sng and max_sng are set to the minimum and maximum string
       values of the first file processed.
       However, we can tell if these strings were artificially generated because 
       nco_lmt_sct_mk() sets the is_usr_spc_lmt flag to False in such cases.
       Subsequent files may have different numbers of records, but nco_lmt_sct_mk()
       is only called once.
       Thus we must update min_idx and max_idx here for each file
       This causes min_idx and max_idx to be out of sync with min_sng and max_sng, 
       which are only set in nco_lmt_sct_mk() for the first file.
       In hindsight, artificially generating min_sng and max_sng may be bad idea */
    /* Following logic is messy, but hard to simplify */
    if(lmt.min_sng == NULL || !lmt.is_usr_spc_lmt){
      /* No user-specified value available--generate minimal dimension index */
      if(FORTRAN_IDX_CNV) lmt.min_idx=1L; else lmt.min_idx=0L;
    }else{
      /* Use user-specified limit when available */
      lmt.min_idx=strtol(lmt.min_sng,(char **)NULL,10);
    } /* end if */
    if(lmt.max_sng == NULL || !lmt.is_usr_spc_lmt){
      /* No user-specified value available---generate maximal dimension index */
      if(FORTRAN_IDX_CNV) lmt.max_idx=dmn_sz; else lmt.max_idx=dmn_sz-1L;
    }else{
      /* Use user-specified limit when available */
      lmt.max_idx=strtol(lmt.max_sng,(char **)NULL,10);
    } /* end if */
    
    /* Adjust indices if FORTRAN style input was specified */
    if(FORTRAN_IDX_CNV){
      lmt.min_idx--;
      lmt.max_idx--;
    } /* end if */
  
    /* Exit if requested indices are always invalid for all operators... */
    if(lmt.min_idx < 0 || lmt.max_idx < 0 || 
       /* ...or are invalid for non-record dimensions or single file operators */
       (!rec_dmn_and_mlt_fl_opr && lmt.min_idx >= dmn_sz)){
      (void)fprintf(stdout,"%s: ERROR User-specified dimension index range %li <= %s <= %li does not fall within valid dimension index range 0 <= %s <= %li\n",prg_nm_get(),lmt.min_idx,lmt.nm,lmt.max_idx,lmt.nm,dmn_sz-1L);
      (void)fprintf(stdout,"\n");
      nco_exit(EXIT_FAILURE);
    } /* end if */
    
    /* Logic depends on whether this is record dimension in multi-file operator */
    if(!rec_dmn_and_mlt_fl_opr || !lmt.is_usr_spc_lmt){
      /* For non-record dimensions and for record dimensions where limit 
	 was automatically generated (to include whole file), starting
	 and ending indices are simply minimum and maximum indices already 
	 in structure */
      lmt.srt=lmt.min_idx;
      lmt.end=lmt.max_idx;
    }else{
      
      /* Allow for possibility initial files are superfluous in multi-file hyperslab */
      if(rec_dmn_and_mlt_fl_opr && cnt_crr == 0L && lmt.min_idx >= dmn_sz+lmt.rec_skp_nsh_spf) flg_no_data=True;
      
      /* Initialize rec_skp_vld_prv to 0L on first call to nco_lmt_evl() 
	 This is necessary due to intrinsic hysterisis of rec_skp_vld_prv
	 rec_skp_vld_prv is used only by multi-file operators
	 rec_skp_vld_prv counts records skipped at end of previous valid file
	 cnt_crr and rec_skp_nsh_spf are both zero only for first file
	 No records were skipped in previous files */
      if(cnt_crr == 0L && lmt.rec_skp_nsh_spf == 0L) lmt.rec_skp_vld_prv=0L;
      
    

      /* This if statement is required to avoid an ugly goto statment */
      if(!flg_no_data){
	
	/* For record dimensions with user-specified limit, allow for possibility 
	   that limit pertains to record dimension in a multi-file operator.
	   Then user-specified maximum index may exceed number of records in any one file
	   Thus lmt.srt does not necessarily equal lmt.min_idx and 
	   lmt.end does not necessarily equal lmt.max_idx */
	/* Stride is officially supported for ncks (all dimensions)
	   and for ncra and ncrcat (record dimension only) */
	if(lmt.srd != 1L && prg_id != ncks && !lmt.is_rec_dmn) (void)fprintf(stderr,"%s: WARNING Stride argument for non-record dimension is only supported by ncks, use at your own risk...\n",prg_nm_get());
	
	/* Note that following block assumes flg_no_data is false
	   Harm would be done if this block were entered while flg_no_data were true
	   What would happen is lmt.end would be set, causing lmt.rec_skp_vld_prv 
	   to be incorrectly set at the end of this block 
	   Note, however, that flg_no_data may be set in two place within this block
	   These occurances are safe because they indicate all data have been read (all following files will be superfluous)
	   Therefore any hinky exit values will not affect future reads
	*/
	if(lmt.is_usr_spc_min && lmt.is_usr_spc_max){
	  /* cnt_rmn_ttl is determined only when both min and max are known */
	  cnt_rmn_ttl=-cnt_crr+1L+(lmt.max_idx-lmt.min_idx)/lmt.srd;
	  if(cnt_rmn_ttl == 0L) flg_no_data=True; /* Safe since all data have been read */
	  if(cnt_crr == 0L){
	    /* Start index is min_idx adjusted for any skipped initial superfluous files */
	    lmt.srt=lmt.min_idx-lmt.rec_skp_nsh_spf;
	    if(lmt.srd == 1L){
	      /* With unity stride, end index is lesser of number of remaining records to read and number of records in this file */
	      lmt.end=(lmt.max_idx < lmt.rec_skp_nsh_spf+dmn_sz) ? lmt.max_idx-lmt.rec_skp_nsh_spf : dmn_sz-1L;
	    }else{
	      cnt_rmn_crr=1L+(dmn_sz-1L-lmt.srt)/lmt.srd;
	      cnt_rmn_crr=(cnt_rmn_ttl < cnt_rmn_crr) ? cnt_rmn_ttl : cnt_rmn_crr;
	      lmt.end=lmt.srt+lmt.srd*(cnt_rmn_crr-1L);
	    } /* end else */
	  }else{
	    /* Records have been read from previous file(s) */
	    if(lmt.srd == 1L){
	      /* Start index is zero since contiguous records are requested */
	      lmt.srt=0L;
	      /* End index is lesser of number of records to read from all remaining files (including this one) and number of records in this file */
	      lmt.end=(cnt_rmn_ttl < dmn_sz) ? cnt_rmn_ttl-1L : dmn_sz-1L;
	    }else{
	      /* Start index will be non-zero if all previous file sizes (in records) were not evenly divisible by stride */
	      lmt.srt=lmt.srd-lmt.rec_skp_vld_prv-1L;
	      cnt_rmn_crr=1L+(dmn_sz-1L-lmt.srt)/lmt.srd;
	      cnt_rmn_crr=(cnt_rmn_ttl < cnt_rmn_crr) ? cnt_rmn_ttl : cnt_rmn_crr;
	      lmt.end=lmt.srt+lmt.srd*(cnt_rmn_crr-1L);
	    } /* end else */
	  } /* endif user-specified records have already been read */
	}else if(lmt.is_usr_spc_min){
	  /* If min was user specified but max was not, then we know which record to 
	     start with and we read every subsequent file */
	  if(cnt_crr == 0L){
	    /* Start index is min_idx adjusted for any skipped initial superfluous files */
	    lmt.srt=lmt.min_idx-lmt.rec_skp_nsh_spf;
	    if(lmt.srd == 1L){
	      lmt.end=dmn_sz-1L;
	    }else{
	      cnt_rmn_crr=1L+(dmn_sz-1L-lmt.srt)/lmt.srd;
	      lmt.end=lmt.srt+lmt.srd*(cnt_rmn_crr-1L);
	    } /* end else */
	  }else{
	    /* Records have been read from previous file(s) */
	    if(lmt.srd == 1L){
	      /* Start index is zero since contiguous records are requested */
	      lmt.srt=0L;
	      lmt.end=dmn_sz-1L;
	    }else{
	      /* Start index will be non-zero if all previous file sizes (in records) were not evenly divisible by stride */
	      lmt.srt=lmt.srd-lmt.rec_skp_vld_prv-1L;
	      cnt_rmn_crr=1L+(dmn_sz-1L-lmt.srt)/lmt.srd;
	      lmt.end=lmt.srt+lmt.srd*(cnt_rmn_crr-1L);
	    } /* end else */
	  } /* endif user-specified records have already been read */
	}else if(lmt.is_usr_spc_max){
	  /* If max was user specified but min was not, then we know which index to 
	     end with and we read record (modulo srd) until we get there */
	  if(cnt_crr == 0L){
	    /* Start index is min_idx = 0L for first file (no initial files are skipped in this case)*/
	    lmt.srt=lmt.min_idx;
	    if(lmt.srd == 1L){
	      /* With unity stride, end index is lesser of number of remaining records to read and number of records in this file */
	      lmt.end=(lmt.max_idx < dmn_sz) ? lmt.max_idx : dmn_sz-1L;
	    }else{
	      /* Record indices in the first file are global record indices */
	      lmt.end=(dmn_sz-(dmn_sz%lmt.srd));
	      lmt.end=(lmt.max_idx < lmt.end) ? lmt.max_idx : lmt.end;
	    } /* end else */
	  }else{
	    /* Records have been read from previous file(s) 
	       We must now account for "index shift" from previous files */
	    
	    long rec_idx_glb_off; /* Global index of first record in this file */
	    long max_idx_lcl; /* User-specified max index in "local" file */
	    
	    /* Global index of first record in this file */
	    rec_idx_glb_off=(cnt_crr-1L)*lmt.srd+lmt.rec_skp_vld_prv+1L;
	    /* Convert user-specified max index to "local" index in current file */
	    max_idx_lcl=lmt.max_idx-rec_idx_glb_off;
	    if(max_idx_lcl < 0) flg_no_data=True; /* Safe since all data have been read */
	    if(lmt.srd == 1L){
	      /* Start index is zero since contiguous records are requested */
	      lmt.srt=0L;
	      lmt.end=(max_idx_lcl < dmn_sz) ? max_idx_lcl : dmn_sz-1L;
	    }else{
	      /* Start index will be non-zero if all previous file sizes (in records) were not evenly divisible by stride */
	      lmt.srt=lmt.srd-lmt.rec_skp_vld_prv-1L;
	      lmt.end=lmt.srt;
	      while(lmt.end < dmn_sz-lmt.srd && lmt.end < max_idx_lcl-lmt.srd){
		lmt.end+=lmt.srd;
	      } /* end while */
	    } /* end else */
	  } /* endif user-specified records have already been read */
	}else if(!lmt.is_usr_spc_min && !lmt.is_usr_spc_max){
	  /* If stride was specified without min or max, then we read in all records
	     (modulo the stride) from every file */
	  if(cnt_crr == 0L){
	    /* Start index is min_idx = 0L for first file (no initial files are skipped in this case)*/
	    lmt.srt=lmt.min_idx;
	    if(lmt.srd == 1L){
	      lmt.end=dmn_sz-1L;
	    }else{
	      lmt.end=(dmn_sz > lmt.srd) ? dmn_sz-1L-(dmn_sz-1L)%lmt.srd : 0L;
	    } /* end else */
	  }else{
	    /* Records have been read from previous file(s) */
	    if(lmt.srd == 1L){
	      /* Start index is zero since contiguous records are requested */
	      lmt.srt=0L;
	      lmt.end=dmn_sz-1L;
	    }else{
	      /* Start index will be non-zero if all previous file sizes (in records) were not evenly divisible by stride */
	      lmt.srt=lmt.srd-lmt.rec_skp_vld_prv-1L;
	      cnt_rmn_crr=1L+(dmn_sz-1L-lmt.srt)/lmt.srd;
	      lmt.end=lmt.srt+lmt.srd*(cnt_rmn_crr-1L);
	    } /* end else */
	  } /* endif user-specified records have already been read */
	} /* end if srd but not min or max was user-specified */
	
	/* Compute diagnostic count for this file only */
	cnt_rmn_crr=1L+(lmt.end-lmt.srt)/lmt.srd;
	/* Save current rec_skp_vld_prv for diagnostics */
	rec_skp_vld_prv_dgn=lmt.rec_skp_vld_prv;
	/* rec_skp_vld_prv for next file is stride minus number of unused records
	   at end of this file (dmn_sz-1L-lmt.end) minus one */
	lmt.rec_skp_vld_prv=dmn_sz-1L-lmt.end;
	/*      assert(lmt.rec_skp_vld_prv >= 0);*/
	
      } /* endif !flg_no_data */
      
    } /* endif user-specified limits to record dimension */
    
  } /* end else limit arguments are hyperslab indices */
  
  /* Compute cnt from srt, end, and srd
     This is fine for multi-file record dimensions since those operators read in one
     record at a time and thus never actually use lmt.cnt for record dimension. */
  if(lmt.srd == 1L){
    if(lmt.srt <= lmt.end) lmt.cnt=lmt.end-lmt.srt+1L; else lmt.cnt=dmn_sz-lmt.srt+lmt.end+1L;
  }else{
    if(lmt.srt <= lmt.end) lmt.cnt=1L+(lmt.end-lmt.srt)/lmt.srd; else lmt.cnt=1L+((dmn_sz-lmt.srt)+lmt.end)/lmt.srd;
  } /* end else */
  
  /* NB: Degenerate cases of WRP && SRD exist for which dmn_cnt_2 == 0
     This occurs when srd is large enough, or max_idx small enough, 
     such that no values are selected in the second read. 
     e.g., "-d lon,60,0,10" if sz(lon)=128 has dmn_cnt_2 == 0
     Since netCDF library reports an error reading and writing cnt=0 dimensions, kludge is necessary
     Syntax ensures it is always the second read, not the first, which is obviated
     Therefore we convert these degenerate cases into non-wrapped coordinates to be processed by single read 
     For these degenerate cases only, [srt,end] are not a permutation of [min_idx,max_idx]
  */
  if(
     (lmt.srd != 1L) && /* SRD */
     (lmt.srt > lmt.end) && /* WRP */
     (lmt.cnt == (1L+(dmn_sz-lmt.srt-1L)/lmt.srd)) && /* dmn_cnt_1 == cnt -> dmn_cnt_2 == 0 */
     True){
    long greatest_srd_multiplier_1st_hyp_slb; /* greatest integer m such that srt+m*srd < dmn_sz */
    long last_good_idx_1st_hyp_slb; /* C index of last valid member of 1st hyperslab (= srt+m*srd) */
    /* long left_over_idx_1st_hyp_slb;*/ /* # elements from first hyperslab to count towards current stride */
    long first_good_idx_2nd_hyp_slb; /* C index of first valid member of 2nd hyperslab, if any */
    
    /* NB: Perform these operations with integer arithmetic or else! */
    /* Wrapped dimensions with stride may not start at idx 0 on second read */
    greatest_srd_multiplier_1st_hyp_slb=(dmn_sz-lmt.srt-1L)/lmt.srd;
    last_good_idx_1st_hyp_slb=lmt.srt+lmt.srd*greatest_srd_multiplier_1st_hyp_slb;
    /*    left_over_idx_1st_hyp_slb=dmn_sz-last_good_idx_1st_hyp_slb-1L;*/
    first_good_idx_2nd_hyp_slb=(last_good_idx_1st_hyp_slb+lmt.srd)%dmn_sz;
    
    /* Conditions causing dmn_cnt_2 == 0 */
    if(first_good_idx_2nd_hyp_slb > lmt.end) lmt.end=last_good_idx_1st_hyp_slb;
  } /* end if */
  
  /* Exit when valid bracketed range contains no coordinates */
  if(lmt.cnt == 0){
    if(lmt.lmt_typ == lmt_crd_val) (void)fprintf(stdout,"%s: ERROR Domain %g <= %s <= %g brackets no coordinate values.\n",prg_nm_get(),lmt.min_val,lmt.nm,lmt.max_val); 
    if(lmt.lmt_typ == lmt_dmn_idx) (void)fprintf(stdout,"%s: ERROR Empty domain for %s\n",prg_nm_get(),lmt.nm); 
    nco_exit(EXIT_FAILURE);
  } /* end if */
  
  /* Coordinate-valued limits that bracket no values in current file jump here with goto
     Index-valued limits with no values in current file flow here naturally */
 no_data: /* end goto */
  if(flg_no_data){
    /* File is superfluous (contributes no data) to specified hyperslab
       Set output parameters to well-defined state
       This state must not cause ncra or ncrcat to retrieve any data
       Since ncra and ncrcat use loops for the record dimension, this
       may be accomplished by returning loop control values that cause
       loop always to be skipped, never entered, e.g., lmt_rec.srt > lmt_rec.end */
    lmt.srt=-1L;
    lmt.end=lmt.srt-1L;
    lmt.cnt=-1L;
    /* Keep track of number of records skipped in initial superfluous files */
    if(cnt_crr == 0L) lmt.rec_skp_nsh_spf+=dmn_sz;
    /* Set variables to preserve utility of diagnostic routines at end of routine */
    cnt_rmn_crr=rec_skp_vld_prv_dgn=lmt.rec_skp_vld_prv=0L;
  } /* endif */
  
  /* Place contents of working structure in location of returned structure */
  *lmt_ptr=lmt;
  
  if(dbg_lvl_get() >= nco_dbg_io){
    (void)fprintf(stderr,"Dimension hyperslabber nco_lmt_evl() diagnostics:\n");
    (void)fprintf(stderr,"Dimension name = %s\n",lmt.nm);
    (void)fprintf(stderr,"Limit type is %s\n",(min_lmt_typ == lmt_crd_val) ? "coordinate value" : "zero-based dimension index");
    (void)fprintf(stderr,"Limit %s user-specified\n",(lmt.is_usr_spc_lmt) ? "is" : "is not");
    (void)fprintf(stderr,"Limit %s record dimension\n",(lmt.is_rec_dmn) ? "is" : "is not");
    (void)fprintf(stderr,"Current file %s specified hyperslab, data %s be read\n",(flg_no_data) ? "is superfluous to" : "is required by",(flg_no_data) ? "will not" : "will");
    if(rec_dmn_and_mlt_fl_opr) (void)fprintf(stderr,"Records skipped in initial superfluous files = %li \n",lmt.rec_skp_nsh_spf);
    if(rec_dmn_and_mlt_fl_opr) (void)fprintf(stderr,"Records read from previous files = %li\n",cnt_crr);
    if(cnt_rmn_ttl != -1L) (void)fprintf(stderr,"Total records to be read from this and all following files = %li\n",cnt_rmn_ttl);
    if(cnt_rmn_crr != -1L) (void)fprintf(stderr,"Records to be read from this file = %li\n",cnt_rmn_crr);
    if(rec_skp_vld_prv_dgn != -1L) (void)fprintf(stderr,"rec_skp_vld_prv_dgn (previous file, if any) = %li \n",rec_skp_vld_prv_dgn);
    if(rec_skp_vld_prv_dgn != -1L) (void)fprintf(stderr,"rec_skp_vld_prv (this file) = %li \n",lmt.rec_skp_vld_prv);
    (void)fprintf(stderr,"min_sng = %s\n",lmt.min_sng == NULL ? "NULL" : lmt.min_sng);
    (void)fprintf(stderr,"max_sng = %s\n",lmt.max_sng == NULL ? "NULL" : lmt.max_sng);
    (void)fprintf(stderr,"srd_sng = %s\n",lmt.srd_sng == NULL ? "NULL" : lmt.srd_sng);
    (void)fprintf(stderr,"min_val = %g\n",lmt.min_val);
    (void)fprintf(stderr,"max_val = %g\n",lmt.max_val);
    (void)fprintf(stderr,"min_idx = %li\n",lmt.min_idx);
    (void)fprintf(stderr,"max_idx = %li\n",lmt.max_idx);
    (void)fprintf(stderr,"srt = %li\n",lmt.srt);
    (void)fprintf(stderr,"end = %li\n",lmt.end);
    (void)fprintf(stderr,"cnt = %li\n",lmt.cnt);
    (void)fprintf(stderr,"srd = %li\n\n",lmt.srd);
  } /* end dbg */
  
  if(lmt.srt > lmt.end && !flg_no_data){
    if(prg_id != ncks) (void)fprintf(stderr,"WARNING: Possible instance of Schweitzer data hole requiring better diagnostics TODO #148\n");
    if(prg_id != ncks) (void)fprintf(stderr,"HINT: If operation fails, try multislabbing (http://nco.sf.net/nco.html#msa) wrapped dimension using ncks first, and then apply %s to the resulting file\n",prg_nm_get());
  } /* end dbg */

  (void*)nco_free(fl_udu_sng);
  
} /* end nco_lmt_evl() */

lmt_sct ** /* O [sct] Structure list with user-specified strings for min and max limits */
nco_lmt_prs /* [fnc] Create limit structures with name, min_sng, max_sng elements */
(const int lmt_nbr, /* I [nbr] number of dimensions with limits */
 CST_X_PTR_CST_PTR_CST_Y(char,lmt_arg)) /* I [sng] List of user-specified dimension limits */
{
  /* Purpose: Set name, min_sng, max_sng elements of 
     comma separated list of names and ranges. This routine
     merely evaluates syntax of input expressions and
     does not attempt to validate dimensions or their ranges
     against those present in input netCDF file. */
  
  /* Valid syntax adheres to nm,[min_sng][,[max_sng][,[srd_sng]]] */
  
  void nco_usg_prn(void);
  
  char **arg_lst;
  
  const char dlm_sng[]=",";
  
  lmt_sct **lmt=NULL_CEWI;
  
  int idx;
  int arg_nbr;
  
  if(lmt_nbr > 0) lmt=(lmt_sct **)nco_malloc(lmt_nbr*sizeof(lmt_sct *));
  for(idx=0;idx<lmt_nbr;idx++){
    /* Process hyperslab specifications as normal text list */
    arg_lst=nco_lst_prs_2D(lmt_arg[idx],dlm_sng,&arg_nbr);
    
    /* Check syntax */
    if(
       arg_nbr < 2 || /* Need more than just dimension name */
       arg_nbr > 4 || /* Too much information */
       arg_lst[0] == NULL || /* Dimension name not specified */
       (arg_nbr == 2 && arg_lst[1] == NULL) || /* No min specified */
       (arg_nbr == 3 && arg_lst[1] == NULL && arg_lst[2] == NULL) || /* No min or max when stride not specified */
       (arg_nbr == 4 && arg_lst[3] == NULL) || /* Stride should be specified */
       False){
      (void)fprintf(stdout,"%s: ERROR in hyperslab specification for dimension %s\n%s: HINT Conform request to hyperslab documentation at http://nco.sf.net/nco.html#hyp\n",prg_nm_get(),lmt_arg[idx],prg_nm_get());
      nco_exit(EXIT_FAILURE);
    } /* end if */
    
    /* Initialize structure */
    /* lmt strings that are not explicitly set by user remain NULL, i.e., 
       specifying default setting will appear as if nothing at all was set.
       Hopefully, in routines that follow, branch followed when dimension has
       all default settings specified (e.g.,"-d foo,,,,") yields same answer
       as branch for which no hyperslab along that dimension was set. */
    lmt[idx]=(lmt_sct *)nco_malloc(sizeof(lmt_sct));
    lmt[idx]->nm=NULL;
    lmt[idx]->is_usr_spc_lmt=True; /* True if any part of limit is user-specified, else False */
    lmt[idx]->min_sng=NULL;
    lmt[idx]->max_sng=NULL;
    lmt[idx]->srd_sng=NULL;
    /* rec_skp_nsh_spf is used for record dimension in multi-file operators */
    lmt[idx]->rec_skp_nsh_spf=0L; /* Number of records skipped in initial superfluous files */
    
    /* Fill in structure */
    lmt[idx]->nm=arg_lst[0];
    lmt[idx]->min_sng=arg_lst[1];
    /* Setting min_sng and max_sng to same pointer would lead to dangerous double-free() condition */
    if(arg_nbr <= 2) lmt[idx]->max_sng=(char *)strdup(arg_lst[1]);
    if(arg_nbr > 2) lmt[idx]->max_sng=arg_lst[2]; 
    if(arg_nbr > 3) lmt[idx]->srd_sng=arg_lst[3];
    
    if(lmt[idx]->max_sng == NULL) lmt[idx]->is_usr_spc_max=False; else lmt[idx]->is_usr_spc_max=True;
    if(lmt[idx]->min_sng == NULL) lmt[idx]->is_usr_spc_min=False; else lmt[idx]->is_usr_spc_min=True;

    /* Initialize types used to re-base coordinate variables */
    lmt[idx]->origin=0.0;
    lmt[idx]->re_bs_sng=NULL_CEWI;

    /* Free current pointer array to strings
       Strings themselves are untouched 
       They will be free()'d with limit structures in nco_lmt_lst_free() */
    arg_lst=(char **)nco_free(arg_lst);

  } /* End loop over lmt structure list */
  
  return lmt;
  
} /* end nco_lmt_prs() */

void
nco_prs_time /* Parse time limits into years, months, day, etc. */
(const char * const in_sng, /* I [sng] String to parse */
 int * const dt, /* O [sct] Time decomposed into longs */
 double * const out_sec) /* O [s] Double precision seconds */
{
  /* Parse limits into years, months, day, etc. */
  char **arg_lst; /* Array of date/time strings after parsing */
  int arg_nbr; /* [nbr] Number of arguments parsed */
  int year,month,day,hour,min;
  double sec;
    
  /* Clear date and time */
  year=0L;
  month=0L;
  day=0L;
  hour=0L;
  min=0L;
  sec=(double)0;
  /* Break up date string copy delimited by dashes */
  arg_lst=nco_lst_prs_2D(in_sng,"-",&arg_nbr);
  /* Fill in year, month, day */
  if((arg_nbr > 0) && (arg_lst[0] != NULL)) year=strtol(arg_lst[0],(char **)NULL,10);
  if((arg_nbr > 1) && (arg_lst[1] != NULL)) month=strtol(arg_lst[1],(char **)NULL,10);
  if((arg_nbr > 2) && (arg_lst[2] != NULL)) day=strtol(arg_lst[2],(char **)NULL,10);
  /* Advance pointer past date/time delimiting space */
  if(arg_lst[2] != NULL) arg_lst[2]=strrchr(arg_lst[2],' ');
  /* Step past last space */
  if(arg_lst[2] != NULL) arg_lst[arg_nbr-1]++;
  /* Break up remaining time string delimited by colons */
  if(arg_lst[2] != NULL){
    arg_lst=nco_lst_prs_2D(arg_lst[2],":",&arg_nbr);
    /* Fill in hour, minute, second */
    if((arg_nbr > 0) && strlen(arg_lst[0]) > 2){ /* HHMM spec */
      hour=strtol(arg_lst[0],(char **)NULL,10);
      hour=(long)(hour/100);
      min=strtol(arg_lst[0],(char **)NULL,10);
      min=(long)(min%100);
    }else{ /* HH:MM:S.S spec */
      if((arg_nbr > 0) && (arg_lst[0] != NULL)) hour=strtol(arg_lst[0],(char **)NULL,10);
      if((arg_nbr > 1) && (arg_lst[1] != NULL)) min=strtol(arg_lst[1],(char **)NULL,10);
      if((arg_nbr > 2) && (arg_lst[2] != NULL)) sec=strtod(arg_lst[2],(char **)NULL);
    } /* end else */
  } /* if arg_lst[2] == NULL */

  arg_lst=nco_sng_lst_free(arg_lst,arg_nbr);

  dt[0]=year;
  dt[1]=month;
  dt[2]=day;
  dt[3]=hour;
  dt[4]=min;
  *out_sec=sec;

} /* end nco_prs_time */

#ifdef ENABLE_UDUNITS
# ifdef HAVE_UDUNITS2_H


int /* [rcd] Successful conversion returns 0 */
nco_lmt_udu_cnv /* [fnc] Convert from Unidata units to coordinate value */
(const int dmn_id, /* I [idx] netCDF dimension ID */
 const char* fl_unt_sng, /* string with disk units attribute */
 char *lmt_sng, /* I [ptr] Limit string */
 double *lmt_val) /* O [val] Limit coordinate value */ 
{
  int ut_rcd; /* [enm] UDUnits2 status */
  char *usr_unt_sng; /* [sng] User-specified unit string */
  double crr_val; 
   
  cv_converter *ut_cnv; /* UDUnits converter */
  ut_system *ut_sys;
  ut_unit *ut_sct_in; /* UDUnits structure, input units */
  ut_unit *ut_sct_out; /* UDUnits structure, output units */
  

  /* When empty, ut_read_xml() uses environment variable UDUNITS2_XML_PATH, if any
     Otherwise it uses default initial location hardcoded when library was built */
  if(dbg_lvl_get() >= nco_dbg_vrb) ut_set_error_message_handler(ut_write_to_stderr); else ut_set_error_message_handler(ut_ignore);
  ut_sys=ut_read_xml(NULL);
  if(ut_sys == NULL){
    (void)fprintf(stdout,"%s: nco_udu_lmt_cnv() failed to initialize UDUnits2 library\n",prg_nm_get());
    return EXIT_FAILURE;
  } /* end if err */ 

  
  /* Convert on-disk 'units' attribute into ut_unit structure */
  ut_sct_out=ut_parse(ut_sys,fl_unt_sng,UT_ASCII); 
  if(ut_sct_out == NULL){ /* Problem with 'units' attribute */
    ut_rcd=ut_get_status(); /* [enm] UDUnits2 status */
    if(ut_rcd == UT_BAD_ARG) (void)fprintf(stderr,"ERROR: empty attribute or units system for dimension ID %d\n",dmn_id);
    if(ut_rcd == UT_SYNTAX) (void)fprintf(stderr,"ERROR: units  attribute \"%s\" for dimension ID %d has a syntax error\n",fl_unt_sng,dmn_id);
    if(ut_rcd == UT_UNKNOWN) (void)fprintf(stderr,"ERROR: units attribute \"%s\" for dimension ID %d is not listed in UDUnits2 SI system database\n",fl_unt_sng,dmn_id);

    return EXIT_FAILURE;
  } /* endif coordinate on disk has no units attribute */



  /* check if disk units is a timestamp if so */
  if( strstr(fl_unt_sng," from ") || strstr(fl_unt_sng," since ") || strstr(fl_unt_sng," after ") ){

  // prefix output units with s@ so that ut_parse works
    if(strncmp("s@", lmt_sng,2)){
      usr_unt_sng=(char*)nco_malloc( (strlen(lmt_sng)+3) *sizeof(char) );
      strcpy(usr_unt_sng,"s@");
      strcat(usr_unt_sng,lmt_sng);
    }else{
      usr_unt_sng=strdup(lmt_sng);
    }


    crr_val=0.0;

  /* assume we are dealing with regular output units */
  }else{
    usr_unt_sng=(char*)nco_calloc( strlen(lmt_sng)+1, sizeof(char));
    /* number space units */
    /* this should be gaurranted by calling function */
    sscanf(lmt_sng,"%lg %s", &crr_val,usr_unt_sng);    
  }



  /* Convert user-specified unit into utUnit structure */
  ut_sct_in=ut_parse(ut_sys,usr_unt_sng,UT_ASCII); 
  if(ut_sct_in == NULL){ /* Problem with 'units' attribute */
    ut_rcd=ut_get_status(); /* [enm] UDUnits2 status */
    if(ut_rcd == UT_BAD_ARG) (void)fprintf(stderr,"ERROR: Empty attribute or units system\n");
    if(ut_rcd == UT_SYNTAX) (void)fprintf(stderr,"ERROR: User-specified unit \"%s\" has a syntax error\n",usr_unt_sng);
    if(ut_rcd == UT_UNKNOWN) (void)fprintf(stderr,"ERROR: User-specified unit \"%s\" is not listed in UDUnits2 SI system database\n",usr_unt_sng);

    return EXIT_FAILURE;
  } /* endif */


  /* Create a converter */
  ut_cnv=ut_get_converter(ut_sct_in,ut_sct_out); /* UDUnits converter */
  if(ut_cnv == NULL){
    ut_rcd=ut_get_status(); /* [enm] UDUnits2 status */
    if(ut_rcd == UT_BAD_ARG) (void)fprintf(stderr,"ERROR: One of units is NULL\n");
    if(ut_rcd == UT_NOT_SAME_SYSTEM) (void)fprintf(stderr,"ERROR: Units belong to different unit systems\n");
    if(ut_rcd == UT_MEANINGLESS) (void)fprintf(stderr,"ERROR: Conversion between user specified unit \"%s\" and file units \"%s\" is  is meaningless\n",usr_unt_sng,fl_unt_sng);

    return EXIT_FAILURE;
  } /* endif */


  /* Finally do the conversion  */
  *lmt_val=cv_convert_double(ut_cnv,crr_val);

  nco_free(usr_unt_sng); 

  ut_free_system(ut_sys); /* Free memory taken by UDUnits library */
  ut_free(ut_sct_in);
  ut_free(ut_sct_out);
  cv_free(ut_cnv);

  return EXIT_SUCCESS; /* Successful conversion */

}  /* end nco_lmt_udu_cnv() */



int   /* O  difference between two co-ordinate units */      
nco_lmt_clc_org(
const char* fl_unt_sng, /* I [ptr] units attribute string from disk  */     
const char* fl_bs_sng,  /* I [ptr] units attribute string from disk  */     
double *og_val)         /* O difference between two units string */
{
  int ut_rcd; /* [enm] UDUnits2 status */
  double in_val=0.0;
      
  cv_converter *ut_cnv; /* UDUnits converter */
  ut_system *ut_sys;
  ut_unit *ut_sct_in; /* UDUnits structure, input units */
  ut_unit *ut_sct_out; /* UDUnits structure, output units */


  /* quick return if units identical */
  if(!strcmp(fl_unt_sng,fl_bs_sng) ){
    *og_val=0.0;  
    return EXIT_SUCCESS;
  }
  /* When empty, ut_read_xml() uses environment variable UDUNITS2_XML_PATH, if any
     Otherwise it uses default initial location hardcoded when library was built */
  if(dbg_lvl_get() >= nco_dbg_vrb) ut_set_error_message_handler(ut_write_to_stderr); else ut_set_error_message_handler(ut_ignore);
  ut_sys=ut_read_xml(NULL);
  if(ut_sys == NULL){
    (void)fprintf(stdout,"%s: nco_udu_lmt_cnv() failed to initialize UDUnits2 library\n",prg_nm_get());
    return EXIT_FAILURE; /* Failure */
  } /* end if err */ 

  
  /* units string to convert from */
  ut_sct_in=ut_parse(ut_sys,fl_unt_sng,UT_ASCII); 
  if(ut_sct_in == NULL){ /* Problem with 'units' attribute */
    ut_rcd=ut_get_status(); /* [enm] UDUnits2 status */
    if(ut_rcd == UT_BAD_ARG) (void)fprintf(stderr,"ERROR: empty units attribute string\n");
    if(ut_rcd == UT_SYNTAX) (void)fprintf(stderr,"ERROR:  units attribute \"%s\" has a syntax error\n",fl_unt_sng);
    if(ut_rcd == UT_UNKNOWN) (void)fprintf(stderr,"ERROR: units attribute \"%s\" is not listed in UDUnits2 SI system database\n",fl_unt_sng);

    return EXIT_FAILURE; /* Failure */
  } /* endif coordinate on disk has no units attribute */




  /* units string to convert to */
  ut_sct_out=ut_parse(ut_sys,fl_bs_sng,UT_ASCII); 
  if(ut_sct_out == NULL){ /* Problem with 'units' attribute */
    ut_rcd=ut_get_status(); /* [enm] UDUnits2 status */
    if(ut_rcd == UT_BAD_ARG) (void)fprintf(stderr,"ERROR: Empty units attribute string\n");
    if(ut_rcd == UT_SYNTAX) (void)fprintf(stderr,"ERROR: units attribute  \"%s\" has a syntax error\n",fl_bs_sng);
    if(ut_rcd == UT_UNKNOWN) (void)fprintf(stderr,"ERROR: units attribute \"%s\" is not listed in UDUnits2 SI system database\n",fl_bs_sng);

      return EXIT_FAILURE; /* Failure */
  } /* endif */


  /* Create a converter */
  ut_cnv=ut_get_converter(ut_sct_in,ut_sct_out); /* UDUnits converter */
  if(ut_cnv == NULL){
    ut_rcd=ut_get_status(); /* [enm] UDUnits2 status */
    if(ut_rcd == UT_BAD_ARG) (void)fprintf(stderr,"ERROR: One of units is NULL\n");
    if(ut_rcd == UT_NOT_SAME_SYSTEM) (void)fprintf(stderr,"ERROR: Units belong to different unit systems\n");
    if(ut_rcd == UT_MEANINGLESS) (void)fprintf(stderr,"ERROR: Conversion between user specified unit \"%s\" and file units \"%s\" is  is meaningless\n",fl_bs_sng,fl_unt_sng);
    return EXIT_FAILURE; /* Failure */
  } /* endif */


  /* Finally do the conversion  */
  *og_val=cv_convert_double(ut_cnv,in_val);
  
  if(dbg_lvl_get() >nco_dbg_std) {
    fprintf(stderr, "%s : nco_lmt_clc_org: difference between systems \"%s\" and \"%s\" is %f\n",prg_nm_get(),fl_unt_sng,fl_bs_sng, *og_val);
  }


  ut_free_system(ut_sys); /* Free memory taken by UDUnits library */
  ut_free(ut_sct_in);
  ut_free(ut_sct_out);
  cv_free(ut_cnv);

  return EXIT_SUCCESS;

}  /* end nco_lmt_clc_org() */



# else /* !HAVE_UDUNITS2_H */

int /* [rcd] Successful conversion returns 0 */
nco_lmt_udu_cnv /* [fnc] Convert from Unidata units to coordinate value */
(const int dmn_id, /* I [idx] netCDF dimension ID */
 const char* fl_unt_sng, /* string with disk units attribute */
 char *lmt_sng, /* I [ptr] Limit string */
 double *lmt_val) /* O [val] Limit coordinate value */ 
{
  int rcd;
  char *usr_unt_sng; /* [sng] User-specified unit string */

  utUnit udu_sct_in; /* UDUnits structure, input units */
  utUnit udu_sct_out; /* UDUnits structure, output units */

# ifdef UDUNITS_PATH
  /* UDUNITS_PATH macro expands to where autoconf found database file */
  rcd=utInit(UDUNITS_PATH);
# else /* !UDUNITS_PATH */
  /* When empty, utInit() uses environment variable UDUNITS_PATH, if any
     Otherwise it uses default initial location hardcoded when library was built */
  rcd=utInit("");
# endif /* !UDUNITS_PATH */

  if(rcd != 0){
    (void)fprintf(stdout,"%s: nco_udu_lmt_cnv() failed to initialize UDUnits library\n",prg_nm_get());

    return EXIT_FAILURE;

  } /* end if err */ 

  /* Convert on-disk 'units' attribute into utUnit structure */
  rcd=utScan(fl_unt_sng,&udu_sct_out); 
  if(rcd == UT_EINVALID){ /* 'units' attribute is absent */
    (void)fprintf(stderr,"ERROR: no units attribute available for dimension %d\n",dmn_id);
    return 1; 
  } /* endif coordinate on disk has no units attribute */
  if(rcd == UT_EUNKNOWN || rcd == UT_ESYNTAX){
    (void)fprintf(stderr,"ERROR units attribute \"%s\" (in input file) is not a valid UDUnits string",fl_unt_sng);
    (void)utTerm(); /* Free memory taken by UDUnits library */
    return 1;
  } /* endif unkown type */

  

  if(utIsTime(&udu_sct_out) && utHasOrigin(&udu_sct_out)){
 

    int dt[5]; /* 0-year, 1-month, 2-day, 3-hour, 4-minute */
    double sec;
    double *crr_val=lmt_val; /* Pointer to set min/max value */
          
    /* Parse limits into years, months, day, etc. */
    (void)nco_prs_time(lmt_sng,dt,&sec);
    

    rcd=utInvCalendar(dt[0],dt[1],dt[2],dt[3],dt[4],sec,&udu_sct_out,crr_val);
    
  }else{ /* if utIsTime() && utHasOrigin */

    double slp; /* Slope */
    double incpt; /* Intercept */
    double crr_val; /* Current value */ 

    /* Parse user-specified unit */
    usr_unt_sng=strchr(lmt_sng,' ');
    /* Advance past space delimiter
       Will not be NULL since nco_lmt_typ() checked for that */
    usr_unt_sng++;
    
    /* Quick return if specified and supplied units are equal */
    if(strcmp(usr_unt_sng,fl_unt_sng) == 0){
      *lmt_val=strtod(lmt_sng,&usr_unt_sng); /* Convert to double */
      (void)utTerm();
      return EXIT_SUCCESS;
    } /* endif */

    rcd=utScan(usr_unt_sng,&udu_sct_in);
    if(rcd == UT_EUNKNOWN){
      (void)fprintf(stderr,"ERROR: User-specified unit \"%s\" is not in UDUnits database.\n",usr_unt_sng);
      (void)utTerm();
      return EXIT_FAILURE;
    } /* endif */

    rcd=utConvert(&udu_sct_in,&udu_sct_out,&slp,&incpt);
    if(rcd == UT_ECONVERT){
      (void)fprintf(stderr,"ERROR: user specified unit \"%s\" cannot be converted into file units \"%s\"\n",usr_unt_sng,fl_unt_sng);
      (void)utTerm();

      return EXIT_FAILURE;
    } /* endif */
    /* Convert to disk-based units */
    crr_val=strtod(lmt_sng,&usr_unt_sng);
    crr_val=slp*crr_val+incpt;
    *lmt_val=crr_val;
    
  } /* !utIsTime() ... */

  (void)utTerm(); /* Free memory taken by UDUnits library */

  return EXIT_SUCCESS; /* Successful conversion */
}  /* end nco_lmt_udu_cnv() */



int                 /* [rcd] Successful conversion returns 0 */
nco_lmt_clc_org(    /* [fnc] Difference between two co-ordinate units */      
const char* fl_unt_sng, /* I [ptr] units attribute string from disk  */     
const char* fl_bs_sng,  /* I [ptr] units attribute string from disk  */     
double *og_val){        /* O [ptr]                                   */

  int rcd;

  double slp;
  double incpt;

  utUnit udu_sct_in; /* UDUnits structure, input units */
  utUnit udu_sct_out; /* UDUnits structure, output units */


  /* quick return if units identical */
  if(!strcmp(fl_unt_sng,fl_bs_sng) ){
    *og_val=0.0;  
    return EXIT_SUCCESS;
  }

# ifdef UDUNITS_PATH
  /* UDUNITS_PATH macro expands to where autoconf found database file */
  rcd=utInit(UDUNITS_PATH);
# else /* !UDUNITS_PATH */
  /* When empty, utInit() uses environment variable UDUNITS_PATH, if any
     Otherwise it uses default initial location hardcoded when library was built */
  rcd=utInit("");
# endif /* !UDUNITS_PATH */

  if(rcd != 0){
    (void)fprintf(stdout,"%s: nco_lmt_cls_org() failed to initialize UDUnits library\n",prg_nm_get());

    return EXIT_FAILURE;

  } /* end if err */ 


  /* units string to convert from */
  rcd=utScan(fl_unt_sng,&udu_sct_in); 
  if(rcd !=0){
    if(rcd == UT_EINVALID)
      (void)fprintf(stderr,"ERROR: units attribute \"%s\" is invalid \n",fl_unt_sng);

    if(rcd == UT_ESYNTAX)
      (void)fprintf(stderr,"ERROR units attribute \"%s\" contains a syntax error",fl_unt_sng);

    if(rcd == UT_EUNKNOWN)
      (void)fprintf(stderr,"ERROR units attribute \"%s\" is not in udunits database",fl_unt_sng);

    (void)utTerm(); /* Free memory taken by UDUnits library */
    return EXIT_FAILURE;
  } /* endif unkown type */


  /* units string to convert to */
  rcd=utScan(fl_bs_sng,&udu_sct_out); 
  if(rcd !=0){
    if(rcd == UT_EINVALID)
      (void)fprintf(stderr,"ERROR: units attribute \"%s\" is invalid \n",fl_bs_sng);

    if(rcd == UT_ESYNTAX)
      (void)fprintf(stderr,"ERROR units attribute \"%s\" contains a syntax error",fl_bs_sng);

    if(rcd == UT_EUNKNOWN)
      (void)fprintf(stderr,"ERROR units attribute \"%s\" is not in udunits database",fl_bs_sng);

    (void)utTerm(); /* Free memory taken by UDUnits library */
    return EXIT_FAILURE;
  } /* endif unkown type */


  rcd=utConvert(&udu_sct_in,&udu_sct_out,&slp,&incpt);
  if(rcd == UT_ECONVERT){
    (void)fprintf(stderr,"ERROR: user specified unit \"%s\" cannot be converted to units \"%s\"\n",fl_unt_sng,fl_bs_sng);
    (void)utTerm();
    return EXIT_FAILURE;
  } /* endif */


           
  *og_val=incpt; 

  /* debug stuff */
  if(dbg_lvl_get() >nco_dbg_std) 
    fprintf(stderr, "%s : nco_lmt_clc_org: difference between systems \"%s\" and \"%s\" is %f\n",prg_nm_get(),fl_unt_sng,fl_bs_sng, *og_val);
  

  (void)utTerm();

  return EXIT_SUCCESS;   

}



# endif /*!HAVE_UDUNITS2 */
#else /* !ENABLE_UDUNITS */

int /* [rcd] Successful conversion returns 0 */
nco_lmt_udu_cnv /* [fnc] Convert from Unidata units to coordinate value */
(const int dmn_id, /* I [idx] netCDF dimension ID */
 const char *fl_unt_sng, /* string with disk units attribute */
 char *lmt_sng, /* I [ptr] Limit string */
 double *lmt_val) /* O [val] Limit coordinate value */ 
{
  int rcd; /* [enm] Return code */

  rcd=(int)(1+0*dmn_id+0*(*lmt_val)); /* CEWI removes unused parameter warnings */
  (void)fprintf(stdout,"UDUnits limit detected in \"%s\" but UDUnits library is unavailable to perform conversion.\nHINT: Re-compile and re-install NCO with UDUnits enabled. http://nco.sf.net/nco.html#udunits",lmt_sng);
  nco_exit(EXIT_FAILURE);
  return EXIT_FAILURE; /* Conversion failed */

}  /* end nco_lmt_udu_cnv() */

int                      /* O  difference between two co-ordinate units */      
nco_lmt_clc_org(
const char* fl_unt_sng, /* I [ptr] units attribute string from disk  */     
const char* fl_bs_sng,  /* I [ptr] units attribute string from disk  */     
double *og_val)         /* O difference between two units string */
{
  (void)fprintf(stdout,"UDUnits limit detected in \"%s\" but UDUnits library is unavailable to perform conversion.\nHINT: Re-compile and re-install NCO with UDUnits enabled. http://nco.sf.net/nco.html#udunits",fl_unt_sng);
  nco_exit(EXIT_FAILURE);
  return EXIT_FAILURE; /* Conversion failed */
} /* end nco_lmt_clc_org() */

#endif /* !ENABLE_UDUNITS */

int /* O [enm] Limit type */
nco_lmt_typ /* [fnc] Determine limit type */
(char *sng) /* I [ptr] Pointer to limit string */
{
  /* Purpose: Determine type of user-specified limit */
  
  /* Test for UDUnits unit string, then for simple coordinate, 
     then date/time string (ie YYYY-mm-DD), else default to dimensional index */
  if(strchr(sng,' ')) /* Space delimits user-specified units */
    return lmt_udu_sng;
  if(strchr(sng,'.') ) /* Decimal point (most common so check first) */
    return lmt_crd_val;
  if(strchr(sng,'E') || strchr(sng,'e') || /* Exponential */
     strchr(sng,'D') || strchr(sng,'d')) /* Double */
    /* Limit is "simple" (non-UDUnits) coordinate value */
    return lmt_crd_val;
  if(strchr(sng,'-') && ((char*)strchr(sng,'-') != (char*)sng)){
    /* check for a date like string */   
    int y,m,d;
    if( sscanf(sng,"%d-%d-%d",&y,&m,&d)==3 ) 
      return lmt_udu_sng;
    else     
      return lmt_crd_val;
  }  
  /* Default: Limit is dimension index */
  return lmt_dmn_idx;

} /* end nco_lmt_typ() */

char * /* O [sng] Units string */
nco_lmt_get_udu_att /* Successful conversion returns units attribute otherwise null */
(const int nc_id, /* I [idx] netCDF file ID */
 const int var_id) /* I [id] Variable ID whose attribute to read */
{
 /* Grab units attribute from disk */
 const char *att_nm="units"; 
 nc_type att_typ; 
 long att_sz;
 char *fl_udu_sng=NULL_CEWI;

 if(nco_inq_att_flg(nc_id,var_id,att_nm,&att_typ,&att_sz) == NC_NOERR){
   /* Allocate memory for attribute */
   if(att_typ == NC_CHAR){
     fl_udu_sng=(char *)nco_malloc((att_sz+1UL)*sizeof(char));
     /* Get 'units' attribute */
     (void)nco_get_att(nc_id,var_id,att_nm,fl_udu_sng,att_typ);
     fl_udu_sng[att_sz]='\0';
   } /* !NC_CHAR */
 } /* endif */
 return fl_udu_sng;
} /* end nco_lmt_get_udu_att() */


