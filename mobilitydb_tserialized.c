/*
 * mobilitydb_tserialized.C 
 *
 * Serialization for the MobilityDB temporal types
 *
 * Author: Maxime Schoemans <maxime.schoemans@ulb.be>
 */

#include <assert.h>

#include <postgres.h>
#include <fmgr.h>
#include <varatt.h>
#include <access/detoast.h>
#include <utils/datetime.h>
#include <utils/date.h>

#include <meos.h>
#include <meos_internal.h>
#include <meos_catalog.h>

#include "mobilitydb_tserialized.h"

PG_MODULE_MAGIC;

#define PG_GETARG_TEMPORAL_P(X)      ((Temporal *) PG_GETARG_VARLENA_P(X))
#define PG_GETARG_TSERIALIZED_P(X)   ((TSERIALIZED *) PG_GETARG_VARLENA_P(X))

#define PG_RETURN_TEMPORAL_P(X)      PG_RETURN_POINTER(X)
#define PG_RETURN_TSERIALIZED_P(X)   PG_RETURN_POINTER(X)

/*****************************************************************************/

static void
error_handler(int errlevel, int errcode, char *errmsg)
{
  elog(errlevel, "%s\n", errmsg);
}

void
_PG_init(void)
{
  meos_initialize(NULL, &error_handler);
  return;
}

void
_PG_fini(void)
{
  meos_finalize();
  return;
}

/*****************************************************************************/

static size_t
tserialized_value_size(const TInstant *temp)
{
  switch (temp->temptype) {
    case T_TINT:
      return sizeof(int32);
    case T_TFLOAT:
      return sizeof(double);
    case T_TBOOL:
      return sizeof(bool);
    case T_TTEXT:
    case T_TGEOMPOINT:
      return sizeof(double) * (2 + MEOS_FLAGS_GET_Z(temp->flags));
    case T_TGEOGPOINT:
      return VARSIZE(&temp->value);
    default:
      elog(ERROR, "Unknown temporal type: %d", temp->temptype);
  }
}

static TSERIALIZED *
tinstant_to_tserialized(TInstant *temp)
{
  TSERIALIZED *result = NULL;
  size_t size = sizeof(TSERIALIZED) + sizeof(TimestampTz);
  size_t v_size = tserialized_value_size(temp);
  size += v_size;
  result = palloc0(size);
  SET_VARSIZE(result, size);
  result->temptype = temp->temptype;
  result->subtype = temp->subtype;
  result->flags = temp->flags;
  memcpy(result->data, &temp->t, sizeof(TimestampTz));
  if (temp->temptype == T_TGEOMPOINT)
  {
    GSERIALIZED *gs = DatumGetGserializedP(tinstant_val(temp));
    memcpy(result->data + sizeof(TimestampTz), GS_POINT_PTR(gs), v_size);
  }
  else
   memcpy(result->data + sizeof(TimestampTz), &temp->value, v_size);
  return result;
}

static TSERIALIZED *
tsequence_to_tserialized(TSequence *temp)
{
  TSERIALIZED *result = NULL;
  size_t size = sizeof(TSERIALIZED) + sizeof(int32) + temp->count * sizeof(TimestampTz);
  const TInstant *inst = TSEQUENCE_INST_N(temp, 0);
  size_t v_size = tserialized_value_size(inst);
  size_t t_offset = sizeof(int32),
         v_offset = sizeof(int32) + temp->count * sizeof(TimestampTz);
  bool is_varlength = basetype_varlength(temptype_basetype(temp->temptype));
  if (temp->temptype == T_TGEOMPOINT)
    is_varlength = false;

  if (is_varlength)
  {
    for (int i = 0; i < temp->count; i++)
      size += VARSIZE(&TSEQUENCE_INST_N(temp, i)->value);
  }
  else
    size += temp->count * v_size;

  result = palloc0(size);
  SET_VARSIZE(result, size);
  result->temptype = temp->temptype;
  result->subtype = temp->subtype;
  result->flags = temp->flags;
  memcpy(result->data, &temp->count, sizeof(int32));
  for (int i = 0; i < temp->count; i++)
  {
    inst = TSEQUENCE_INST_N(temp, i);
    if (is_varlength)
      v_size = VARSIZE(&inst->value);
    memcpy(result->data + t_offset, &inst->t, sizeof(TimestampTz));
    if (temp->temptype == T_TGEOMPOINT)
    {
      GSERIALIZED *gs = DatumGetGserializedP(tinstant_val(inst));
      memcpy(result->data + v_offset, GS_POINT_PTR(gs), v_size);
    }
    else
      memcpy(result->data + v_offset, &inst->value, v_size);
    t_offset += sizeof(TimestampTz);
    v_offset += v_size;
  }
  return result;
}

static TSERIALIZED *
tsequenceset_to_tserialized(TSequenceSet *temp)
{
  TSERIALIZED *result = NULL;
  size_t size;
  switch (temp->temptype) {
    case T_TINT:
      break;
    case T_TFLOAT:
      break;
    case T_TBOOL:
      break;
    case T_TTEXT:
      break;
    case T_TGEOMPOINT:
    case T_TGEOGPOINT:
      break;
    default:
      elog(ERROR, "Unknown temporal type: %d", temp->temptype);
  }
  return result;
}

TSERIALIZED *
temporal_to_tserialized(Temporal *temp)
{
  TSERIALIZED *result;
  switch (temp->subtype) {
    case TINSTANT:
      result = tinstant_to_tserialized((TInstant *) temp);
      break;
    case TSEQUENCE:
      result = tsequence_to_tserialized((TSequence *) temp);
      break;
    case TSEQUENCESET:
      result = tsequenceset_to_tserialized((TSequenceSet *) temp);
      break;
    default:
      elog(ERROR, "Unknown temporal subtype: %d", temp->subtype);
  }
  return result;
}

static GSERIALIZED *
geopoint_make_fast(double x, double y, double z, bool hasz, bool geodetic,
  int32 srid)
{
  GSERIALIZED *g = NULL;
  uint8 *ptr = NULL;
  size_t size = 16 + sizeof(double) * (hasz ? 3 : 2);
  int type = 1; // POINTTYPE
  int npoints = 1;

  ptr = palloc0(size);
  g = (GSERIALIZED*)(ptr);
  SET_VARSIZE(g, size);

  /* Set flags */
  g->gflags = 0x40; // Set GSERIALIZED Version bit
  FLAGS_SET_Z(g->gflags, hasz);
  FLAGS_SET_GEODETIC(g->gflags, geodetic);

  /* Set SRID (use postgis function in meos) */
  // gserialized2_set_srid(g, geom->srid);
  g->srid[0] = (srid & 0x001F0000) >> 16;
  g->srid[1] = (srid & 0x0000FF00) >> 8;
  g->srid[2] = (srid & 0x000000FF);

  /* Move write head past size, srid and flags. */
  ptr += 8;

  /* Write in the type. */
  memcpy(ptr, &type, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  /* Write in the number of points (0 => empty). */
  memcpy(ptr, &npoints, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  /* Copy in the ordinates. */
  memcpy(ptr, &x, sizeof(double));
  ptr += sizeof(double);
  memcpy(ptr, &y, sizeof(double));
  ptr += sizeof(double);
  if (hasz)
    memcpy(ptr, &z, sizeof(double));

  return g;
}

static TInstant *
tserialized_to_tinstant(TSERIALIZED *ts)
{
  TInstant *result = NULL;
  Timestamp t = *(TimestampTz *)(ts->data);
  Datum value;
  GSERIALIZED *g = NULL;
  int32 srid = 0; // TODO: save and restore SRID
  POINT3D *p = NULL;
  bool hasz = MEOS_FLAGS_GET_Z(ts->flags);
  switch (ts->temptype) {
    case T_TINT:
      value = Int32GetDatum(*(int32 *)(ts->data + sizeof(TimestampTz)));
      break;
    case T_TFLOAT:
      value = Float8GetDatum(*(double *)(ts->data + sizeof(TimestampTz)));
      break;
    case T_TBOOL:
      value = BoolGetDatum(*(bool *)(ts->data + sizeof(TimestampTz)));
      break;
    case T_TTEXT:
      value = PointerGetDatum((text *)(ts->data + sizeof(TimestampTz)));
      break;
    case T_TGEOMPOINT:
      srid = 0; // TODO: save and restore SRID
      p = (POINT3D *)(ts->data + sizeof(TimestampTz));
      g = geopoint_make_fast(p->x, p->y, hasz ? p->z : 0, 
        hasz, MEOS_FLAGS_GET_GEODETIC(ts->flags), srid);
      value = PointerGetDatum(g);
      break;
    case T_TGEOGPOINT:
      value = PointerGetDatum((GSERIALIZED *)(ts->data + sizeof(TimestampTz)));
      break;
    default:
      elog(ERROR, "Unknown temporal type: %d", ts->temptype);
  }
  result = tinstant_make(value, ts->temptype, t);
  if (ts->temptype == T_TGEOMPOINT)
    pfree(g);
  return result;
}

static TSequence *
tserialized_to_tsequence(TSERIALIZED *ts)
{
  TSequence *result = NULL;
  int count = *(int32 *)(ts->data);
  TInstant **instants = palloc(sizeof(TInstant *) * count);
  size_t t_offset = sizeof(int32), 
         v_offset = sizeof(int32) + count * sizeof(TimestampTz);
  Timestamp t;
  Datum value;
  GSERIALIZED *g = NULL;
  int32 srid = 0; // TODO: save and restore SRID
  POINT3D *p = NULL;
  bool hasz = MEOS_FLAGS_GET_Z(ts->flags);
  for (int i = 0; i < count; i++)
  {
    t = *(TimestampTz *)(ts->data + t_offset);
    switch (ts->temptype) {
      case T_TINT:
        value = Int32GetDatum(*(int32 *)(ts->data + v_offset));
        v_offset += sizeof(int32);
        break;
      case T_TFLOAT:
        value = Float8GetDatum(*(double *)(ts->data + v_offset));
        v_offset += sizeof(double);
        break;
      case T_TBOOL:
        value = BoolGetDatum(*(bool *)(ts->data + v_offset));
        v_offset += sizeof(bool);
        break;
      case T_TTEXT:
        value = PointerGetDatum((text *)(ts->data + v_offset));
        v_offset += VARSIZE(value);
        break;
      case T_TGEOMPOINT:
        srid = 0; // TODO: save and restore SRID
        p = (POINT3D *)(ts->data + v_offset);
        g = geopoint_make_fast(p->x, p->y, hasz ? p->z : 0, 
          hasz, MEOS_FLAGS_GET_GEODETIC(ts->flags), srid);
        value = PointerGetDatum(g);
        v_offset += sizeof(double) * (hasz ? 3 : 2);
        break;
      case T_TGEOGPOINT:
        value = PointerGetDatum((GSERIALIZED *)(ts->data + v_offset));
        v_offset += VARSIZE(&value);
        break;
      default:
        elog(ERROR, "Unknown temporal type: %d", ts->temptype);
    }
    instants[i] = tinstant_make(value, ts->temptype, t);
    t_offset += sizeof(TimestampTz);
    if (ts->temptype == T_TGEOMPOINT)
      pfree(g);
  }
  result = tsequence_make_free(instants, count, true, true, MEOS_FLAGS_GET_INTERP(ts->flags), false);
  return result;
}

Temporal *
tserialized_to_temporal(TSERIALIZED *ts)
{
  Temporal *result = NULL;
  switch (ts->subtype) {
    case TINSTANT:
      result = (Temporal *) tserialized_to_tinstant(ts);
      break;
    case TSEQUENCE:
      result = (Temporal *) tserialized_to_tsequence(ts);
      break;
    case TSEQUENCESET:
      break;
    default:
      elog(ERROR, "Unknown temporal subtype: %d", ts->subtype);
  }
  return result;
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(TSerialized_in);
Datum
TSerialized_in(PG_FUNCTION_ARGS)
{
  elog(ERROR, 
    "%s is currently not implemented for type tserialized", __func__);
  PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(TSerialized_out);

Datum
TSerialized_out(PG_FUNCTION_ARGS)
{
  elog(ERROR, 
    "%s is currently not implemented for type tserialized", __func__);
  PG_RETURN_NULL();
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(TSerialized_from_temporal);
Datum
TSerialized_from_temporal(PG_FUNCTION_ARGS)
{
  Temporal *temp = PG_GETARG_TEMPORAL_P(0);
  TSERIALIZED *result = temporal_to_tserialized(temp);
  PG_RETURN_TSERIALIZED_P(result);
}

PG_FUNCTION_INFO_V1(TSerialized_to_temporal);
Datum
TSerialized_to_temporal(PG_FUNCTION_ARGS)
{
  TSERIALIZED *ts = PG_GETARG_TSERIALIZED_P(0);
  Temporal *result = tserialized_to_temporal(ts);
  PG_RETURN_POINTER(result);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(TSerialized_mem_size);
Datum
TSerialized_mem_size(PG_FUNCTION_ARGS)
{
  Datum result = toast_raw_datum_size(PG_GETARG_DATUM(0));
  PG_RETURN_DATUM(result);
}

static int
tserialized_num_instants(const TSERIALIZED *ts)
{
  switch (ts->subtype)
  {
    case TINSTANT:
      return 1;
    default:
      return *(int32 *)(ts->data);
  }
}

PG_FUNCTION_INFO_V1(TSerialized_num_instants);
Datum
TSerialized_num_instants(PG_FUNCTION_ARGS)
{
  TSERIALIZED *ts = PG_GETARG_TSERIALIZED_P(0);
  int result = tserialized_num_instants(ts);
  PG_FREE_IF_COPY(ts, 0);
  PG_RETURN_INT32(result);
}

/*****************************************************************************/