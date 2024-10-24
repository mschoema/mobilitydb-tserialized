#ifndef __TEMPORAL_H__
#define __TEMPORAL_H__

#include <stdint.h>

#include <meos.h>

typedef struct
{
  int32 vl_len_;        /* Varlena header (do not touch directly!) */
  uint8 temptype;       /* Temporal type */
  uint8 subtype;        /* Temporal subtype */
  int16 flags;          /* Flags */
  uint8 data[];         /* Variable-length data */
} TSERIALIZED;

extern TSERIALIZED *temporal_to_tserialized(Temporal *temp);
extern Temporal *tserialized_to_temporal(TSERIALIZED *ts);

#endif