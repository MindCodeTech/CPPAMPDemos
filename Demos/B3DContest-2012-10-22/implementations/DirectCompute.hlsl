
#define	SIMDsize		4
#define FLT_MAX			3.402823466e+38F        /* max value */
#define DBL_MAX			3.402823466e+38F        /* max value */

#define	sumtype			float
#define	sumvectype		float4
#define	idxtype			int
#define	idxvectype		int4
#define	maxvec			FLT_MAX
#define	tileheight		2
#define	tilestride		512

#include "DirectCompute-Helper.hlsl"

struct gpuio {
      sumtype currLambda;
      uint currLambdaCount;
      sumtype currLowerBound;
      int currLowestSubgraph;
};

RWStructuredBuffer<gpuio> persistant;

Texture2D<uint4> edgematrix;
Texture2D<uint> included;

#ifndef	CACHEABLE
RWStructuredBuffer<sumvectype> wghtmatrix;
RWStructuredBuffer<sumvectype> weights;
RWStructuredBuffer<sumvectype> mstree;
RWStructuredBuffer<idxvectype> parent;
RWStructuredBuffer<idxvectype> connections;
#else
RWStructuredBuffer<sumvectype> wghtmatrix;
groupshared sumvectype weights[tilestride];
groupshared sumvectype mstree[tilestride];
groupshared idxvectype parent[tilestride];
groupshared idxvectype connections[tilestride];
#endif

RWTexture1D<uint4> parents;

extern sumtype bestLowerBound;

extern sumtype lambda;
extern sumtype lambda_reduction;
extern sumtype lambda_termination;

//#define	numnodes	500
//#define	edgestride	508
//#define	edgesimds	512
extern uint numnodes;
extern uint edgestride;
extern uint edgesimds;

groupshared uint4      cv[tilestride >> 1];
groupshared sumvectype mv[tilestride >> 1];

/* ********************************************************************************************
 */

/* execute a block of code only once,
 * use thread "0" for that
 */
#define firstthread(n)					\
   (((tileheight == 1) || (!th.y)) && !th.x)

#define singlethread(n, func)				\
  if ((tileheight == 1) || (!th.y)) {			\
    if ((n) == th.x) {					\
      const uint j = n;					\
      func						\
    }							\
  }

/* execute a block of code some chosen times
 */
#define limitedthread(i, n, func)			\
  if ((tileheight == 1) || (!th.y)) {			\
    if (th.x < (n)) {					\
      const uint i = th.x;				\
      func						\
    }							\
  }

/* execute a block of code some chosen times
 */
#define wfrontthread(w, func)				\
  if ((tileheight == 1) || (!th.y)) {			\
    if (th.x < WAVEFRONT_SIZE) {			\
      const uint w = th.x;				\
      func						\
    }							\
  }

/* execute a block of code over the stride of numnodes,
 * edgesimds is the valid memory region,
 * do this in parallel
 */
#define rangedthread(l, h, func)			\
  if ((tileheight == 1) || (!th.y)) {			\
    if ((th.x >= (l)) && (th.x < (h))) {		\
      const uint j = th.x;				\
      func						\
    }							\
  }

/* execute a block of code on all threads in parallel
 */
#define allthread(a, func)				\
  if (1) {						\
    const uint a = th.x;				\
    func						\
  }

/* execute a block of code over the stride of numnodes,
 * edgesimds is the valid memory region,
 * do this in parallel
 */
#define clampstride(i, e, func)				\
  if (th.x < (e)) {					\
    const uint i = th.x;				\
    func						\
  }

/* execute a block of code over the size of a wave-front,
 * do this in parallel, any "wavefront-local" conditional
 * won't slow down the code as the entire wavefront sees
 * the same branch(es)
 * this runs in parallel per tileheight (no check)
 */
#define clampwfront(w, func)				\
  if (th.x < WAVEFRONT_SIZE) {				\
    const uint w = th.x;				\
    func						\
  }

/* execute a block of code over multiple strides of numnodes,
 * edgesimds is the valid memory region,
 * do this in parallel
 */
#define pfrontthread(w, s, e, numnodes, func)		\
  for (uint pt = (s); pt < (e); pt += tileheight) {	\
    const uint w = pt + th.y;				\
    if (w < (numnodes)) {				\
      func						\
    }							\
  }

/* perform a binary reduction, first execute the seeding
 * block of code, then successively apply the reducing
 * block of code until all data is consumed
 */
#define reducethread(max, cond, init, seed, reduce) {	\
  /* globally valid thread-to-location mappings */	\
  const uint i  = (th.x << 0) + 0;			\
  const uint i0 = (th.x << 1) + 0;			\
  const uint i1 = (th.x << 1) + 1;			\
  							\
  const uint pos0 = (i0 * SIMDsize);			\
  const uint pos1 = (i1 * SIMDsize);			\
 							\
  const uint4 curindex0 = uint4(pos0 + 0, pos0 + 1, pos0 + 2, pos0 + 3);			\
  const uint4 curindex1 = uint4(pos1 + 0, pos1 + 1, pos1 + 2, pos1 + 3);			\
 							\
  uint length = ((max) + 1) >> 1; {			\
    if (((tileheight == 1) || (!th.y)) && (cond)) {	\
      if ((th.x < length))				\
        seed						\
      if ((th.x == length) && (length & 1))		\
        init						\
    }							\
    							\
    /* ensure "half" has been calculated */		\
    yield(th, 1);					\
  }							\
  							\
  while ((length = (length + 1) >> 1) > 1) {		\
    if (((tileheight == 1) || (!th.y)) && (cond)) {	\
      if ((th.x < length))				\
	reduce						\
      if ((th.x == length) && (length & 1))		\
        init						\
    }							\
  							\
    /* ensure "quarter" has been calculated */		\
    yield(th, 2);					\
  }							\
							\
  if (((tileheight == 1) || (!th.y)) && (cond)) {	\
    /* no yield necessary if wave-frontsize >= 2 */	\
    if (th.x < 1)					\
      reduce						\
  }}							\
							\
    yield(th, 0);

/* ********************************************************************************************
 */

  void yield(uint3 th, uint substride = 0) {
    if ((tilestride >> substride) > WAVEFRONT_SIZE)
      GroupMemoryBarrierWithGroupSync();
    else
      GroupMemoryBarrier();
  }

  void yieldall(uint3 th) {
    if ((tilestride * tileheight) > WAVEFRONT_SIZE)
      GroupMemoryBarrierWithGroupSync();
    else
      GroupMemoryBarrier();
  }

  void broadcast(uint3 th, bool istatic = false) {
    if (((tilestride * 1) > WAVEFRONT_SIZE) && (!istatic))
      AllMemoryBarrier();
    else
      GroupMemoryBarrier();
  }

/* ********************************************************************************************
 */

  void clear(uint3 th) {
    /* mask out over-aligned weights */
    sumvectype l = sumvectype(
      (3 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0),
      (2 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0),
      (1 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0),
      (0 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0)
    );

    /* if "numnodes % SIMDsize" == 0, then "numnodes / SIMDsize" == edgesimds (< j) */
    limitedthread(j, (edgesimds), {
      weights[j] = (j == (numnodes / SIMDsize) ? l : 0);
    });
  }

  void initialize1(uint3 th, uint firstNeighbor) {
    /* ensure "firstNeighbor" has been calculated */
    yieldall(th);

    limitedthread(j, edgesimds, {
      mstree[j] = wghtmatrix[firstNeighbor * edgestride + j];
    });
  }

  /* --------------------------------------------------------------------------------------
   */

  void initialize2(uint3 th, uint firstNeighbor) {
    /* mask out over-aligned connections */
    idxvectype l = idxvectype(
      (3 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0),
      (2 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0),
      (1 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0),
      (0 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0)
    );

    /* if "numnodes % SIMDsize" == 0, then "numnodes / SIMDsize" == edgesimds (< j) */
    limitedthread(j, edgesimds, {
      parent     [j] = firstNeighbor;
      connections[j] = (j == (numnodes / SIMDsize) ? l : 0);
    });
  }

  void connect(uint i, uint j, inout sumtype lowerBound) {
    /* indexed access to vector */
    lowerBound += wghtmatrix[i * edgestride + j / SIMDsize][j % SIMDsize];

    /* register new connections */
    connections[i / SIMDsize][i % SIMDsize]++;
    connections[j / SIMDsize][j % SIMDsize]++;
  }

  groupshared uint ll;
  groupshared uint hh;

  void computeOneTree(
    uint3 th,
    inout sumtype lowerBound,
    inout int lowestSubgraph
  ) {
    /* divide into 4/32/64 buckets (wavefront size) */
//    tile_static struct amphelper::ampminima<sumvectype, int_4, tilestride> omi;

    /* VS is very bad at aliasing variables into thread-local storage
     * also no reference-casting, no unions (because of non-trivial
     * constructor ...), bad situation!
     */
//  tile_static int_4      cv[tilestride >> 1];
//  tile_static int_4      lv[tilestride >> 1];
//  tile_static sumvectype mv[tilestride >> 1];

    /* ensure "weights" has been calculated */
    yield(th);

    /* compute adjusted costs (all rows)
     *
     * this is the only block where the excess threads (> tilestride)
     * are used at all, after this block they'll basically run idle
     */
    pfrontthread(row, 0, numnodes, numnodes, {
      /* vertical */
      sumvectype vwght = weights[row / SIMDsize][row % SIMDsize];

      /* burst reads for each tile-layout case:
       *
       *  1024x1 ... 32x32 (4-way double) -> 32768 bytes ->  16 bursts of (8 * 256 bytes) -> all channels used
       *  1024x1 ... 32x32 (4-way float ) -> 16384 bytes ->   8 bursts of (8 * 256 bytes)
       *  1024x1 ... 32x32 (4-way ulong ) -> 16384 bytes ->   8 bursts of (8 * 256 bytes)
       *  1024x1 ... 32x32 (4-way ushort) ->  8192 bytes ->   4 bursts of (8 * 256 bytes)
       *  1024x1 ... 32x32 (4-way uchar ) ->  4096 bytes ->   2 bursts of (8 * 256 bytes)
       *
       *  16x16            (4-way double) ->  8192 bytes ->  4/1 burst of (8 * 256 bytes)
       *  16x16            (4-way float ) ->  4096 bytes ->  2/1 burst of (8 * 256 bytes)
       *  16x16            (4-way ulong ) ->  4096 bytes ->  2/1 burst of (8 * 256 bytes)
       *  16x16            (4-way ushort) ->  2048 bytes ->  1/1 burst of (8 * 256 bytes)
       *  16x16            (4-way uchar ) ->  1024 bytes ->  1/2 burst of (8 * 256 bytes)
       *
       *  8x8              (4-way double) ->  2048 bytes ->  1/1 burst of (8 * 256 bytes)
       *  8x8              (4-way float ) ->  1024 bytes ->  1/2 burst of (8 * 256 bytes)
       *  8x8              (4-way ulong ) ->  1024 bytes ->  1/2 burst of (8 * 256 bytes)
       *  8x8              (4-way ushort) ->   512 bytes ->  1/4 burst of (8 * 256 bytes)
       *  8x8              (4-way uchar ) ->   256 bytes ->  1/8 burst of (8 * 256 bytes)
       *
       * texture (slow path): uchar/ushort/ulong
       * array   (fast path): float/double
       */
      clampstride(c, edgesimds, {
      	/* rotate the access pattern in case multiple wavefront/tile_rows
      	 * are active to prevent memory channel bank conflicts
      	 */
      	uint col = (c + row) % (edgesimds);

	/* horizontal */
        sumvectype hwght = weights[col];

	uint pos = (col * SIMDsize);
	uint basebit = pos & 31;
	uint bitfield = included[uint2(row, pos >> 5)];

	sumvectype wght;

	// wghtmatrix[row * edgestride + col] = edgematrix[row * edgestride + col] + weights[row] + weights[col];
	wght = sumvectype(edgematrix[uint2(row, col)]) + (vwght + hwght);
	wght = setenabled(wght, idxvectype(
	  bitfield & (1 << (basebit + 0)),
	  bitfield & (1 << (basebit + 1)),
	  bitfield & (1 << (basebit + 2)),
	  bitfield & (1 << (basebit + 3))
	), maxvec);

	wghtmatrix[row * edgestride + col] = wght;
      });
    });

    /* ensure "wghtmatrix[0]" has been calculated */
    yield(th);

    /* horizontal reduction (X to 4/32/64, vectorized, single wave-front) */
    wfrontthread(w, {
      init(w, maxvec);

      /* start & end, clamped to edgestride */
      uint l = min((w + 0) * blcksize, edgesimds);
      uint h = min((w + 1) * blcksize, edgesimds);
      for (uint col = l; col < h; col++) {
        uint pos = (col * SIMDsize);

        uint4 curindex = uint4(pos + 0, pos + 1, pos + 2, pos + 3);
        sumvectype wght = wghtmatrix[0 * edgestride + col];

        /* assign to one of the 4/32/64 buckets */
        orderedmin(w, wght, curindex);
      }
    });

    /* get the two cheapest edges from 0 --------------------------------------------------- */
    uint firstNeighbor  = 0;
    uint secondNeighbor = 0;

    /* vertical reduction (4/32/64 to 2, then 2 to 1, vectorized, single wave-front) */
//  limitedthread(w, 2, { orderedred2(edgesimds, w); });
    if ((tileheight == 1) || !th.y) {
      if (th.x < (2)) {
        const uint w = th.x;
        orderedred2(edgesimds, w);
      }
    }

//  singlethread (   0, { orderedred1(edgesimds   ); });
    if ((tileheight == 1) || !th.y) {
      if (th.x == (0)) {
        orderedred1(edgesimds);
      }
    }

    /* ensure "omi" is visible to other threads/wavefronts (1:N) */
    broadcast(th, true /* == tile_static */);

    /* horizontal reduction (4-way vector to scalar, suck from thread-local into registers) */
    orderedmallest(firstNeighbor, secondNeighbor);

    /* initialize tree */
    initialize2(th, firstNeighbor);

    /* compute the minimum spanning tree on nodes [1, numnodes - 1] ------------------------ */
    lowerBound = 0;

    /* copy out initial costs from weight-matrix (waits for "wghtmatrix" being valid) */
    initialize1(th, firstNeighbor);

    /* calculate length of path */
    if (firstthread(0)) {
      /* add first edge */
      connect(0, firstNeighbor, lowerBound);
      parent[firstNeighbor / SIMDsize][firstNeighbor % SIMDsize] = 0;
    }

    /* ensure "connect[]" and "parent[]" is visible to other threads/wavefronts */
    broadcast(th, false /* == memory */);

    /* consume all unassigned edges */
    for (uint k = 2, l = 0, h = (edgesimds); k < (numnodes /* * th.y */); k++) {
      /* search first unassigned with lowest cost  - - - - - - - - - - - - - - - - - - - - - */

      /* ensure "parent[]" and "mstree[]" has been calculated */
      yield(th);

#ifdef	CACHEABLE
      /* l & h is the same for all threads now */
      if (firstthread(0)) {
        hh = l;
        ll = h;
      }
#endif

      reducethread(h, 1, {
	/* pad binary reduction "overflow" */
	mv[length] = maxvec;
      }, {
	/* excess connections are neutral "2" */
	uint4 con0 = connections[i0];
	uint4 con1 = connections[i1];
	idxvectype cv_i_ = 0;
	sumvectype mv_i_ = maxvec;

	/* skip i/o if not necessary (all non-zero) */
	uint4 chk;

	chk.xy = con1.xy * con1.zw;
	chk.zw = con0.xy * con0.zw;
	chk.x  = chk.x * chk.y;
	chk.z  = chk.z * chk.w;

	if (!(chk.x * chk.z)) {
	  /* skip connections of "> 0" (assign MAX, voids the "smaller") */
	  sumvectype mstv0 = mstree[i0];
	  sumvectype mstv1 = mstree[i1];

	  mstv0 = setdisabled(mstv0, con0, cv_i_);
	  mstv1 = setdisabled(mstv1, con1, mv_i_);

	  /* maintain lower index in case of equality */
	  cv_i_ = smalleri(mstv1, mstv0, curindex1, curindex0);
	  mv_i_ = smallers(mstv1, mstv0,     mstv1,     mstv0);

#ifdef	CACHEABLE
	  /* i1 > i0 */
	  if (!chk.x) {
	    InterlockedMin(ll, i1);
	    InterlockedMax(hh, i1);
	  }

	  if (!chk.z) {
	    InterlockedMin(ll, i0);
	    InterlockedMax(hh, i0);
	  }
#endif
	}

	cv[i] = cv_i_;
	mv[i] = mv_i_;
      }, {
	cv[i] = smalleri(mv[i1], mv[i0], cv[i1], cv[i0]);
	mv[i] = smallers(mv[i1], mv[i0], mv[i1], mv[i0]);
      });

#ifdef	CACHEABLE
      l = ll + 0;
      h = hh + 1;
#endif

      /* add unassigned with lowest cost - - - - - - - - - - - - - - - - - - - - - - - - - - */
      uint i = usmallest(cv[0], mv[0]);

      if (firstthread(0)) {
        connect(parent[i / SIMDsize][i % SIMDsize], i, lowerBound);
      }

      /* ensure "connect[]" and "parent[]" is visible to other threads/wavefronts */
      broadcast(th, false /* == memory */);

      /* reassign costs  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
      uint4 iv = i;

      rangedthread(l, h, {
	uint pos = (j * SIMDsize);

	/* excess connections are neutral "2" */
	uint4 curindex = uint4(pos + 0, pos + 1, pos + 2, pos + 3);
	uint4 con = connections[j];

	/* skip i/o if not necessary (all non-zero) */
	uint2 chk = con.xy * con.zw;
	if (!(chk.x * chk.y)) {
	  /* skip connections of "> 0" (assign MAX, voids the "greater") */
	  sumvectype wght = wghtmatrix[i * edgestride + j];
	  sumvectype mstv = mstree[j];

          /* GPUs have vectorized cmov */
	  wght = setdisabled(wght, con, maxvec);

	  /* maintain lower index in case of equality */
	  parent[j] = greateri(mstv, wght, iv, parent[j]);
	  mstree[j] = greaters(mstv, wght, wght, mstv);
        }
      });
    }

    /* add last edge */
    if (firstthread(0)) {
      connect(0, secondNeighbor, lowerBound);
      parent[0][0] = secondNeighbor;
    }

    /* ensure "connect[]" and "parent[]" is visible to other threads/wavefronts */
    broadcast(th, false /* == memory */);

    /* built shortest non-cycle sub-graph -------------------------------------------------- */
    reducethread(edgesimds, 1, {
      /* pad binary reduction "overflow" */
      mv[length] = 0x7FFF0000;
    },{
      /* excess connections are neutral "2" */
      idxvectype con0 = (connections[i0] << 16) + 0x7FFD0000; /* -2 wraparound signflip */
      idxvectype con1 = (connections[i1] << 16) + 0x7FFD0000; /* -2 wraparound signflip */

      /* maintain lower index in case of equality */
      idxvectype ab = smallerii(con1, con0, curindex1, curindex0);
      idxvectype cd = smallerii(con1, con0,      con1,      con0);

      cv[i] = ab;
      mv[i] = cd;
    }, {
      cv[i] = smalleri(mv[i1], mv[i0], cv[i1], cv[i0]);
      mv[i] = smallers(mv[i1], mv[i0], mv[i1], mv[i0]);
    });

    /* sync-free, wave-fronts are ordered after "reducethread" */
    if (firstthread(0)) {
      /* broadcast lowerBound to all threads */
      mv[1].x = lowerBound;
    }

    broadcast(th, true /* == tile_static */);

    /* round-to-nearest, round half towards minus infinity, prevent sum-of-squares problem */
    lowerBound = round(mv[1].x);
    lowestSubgraph = usmallestneg(cv[0], mv[0]);
  }

[numthreads(tilestride, tileheight, 1)]
void main(
	uint3 th : SV_GroupThreadID
)
{
	/* read persistant GPU-data into registers
	 * maintain across-call coherency, don't use constant buffers
	 */
	sumtype currLambda         = persistant[0].currLambda;
	uint    currLambdaCount    = persistant[0].currLambdaCount;
	sumtype prevLowerBound     = persistant[0].currLowerBound;
	int     currLowestSubgraph = persistant[0].currLowestSubgraph;
//	sumtype bestLowerBound     = persistant[0].bestLowerBound;
//	sumtype currLowerBound     = persistant[0].currLowerBound;
	sumtype currLowerBound     = 0;

	/* prepare weights (initially zero) */
	if (!currLowestSubgraph)
	  clear(th);

//while (lambda > lambda_termination)
	{
	  /* interpretes:
	   * - weights (all of it)
	   * - edgematrix (all of it)
	   *
	   * fills:
	   * - wghtmatrix (all of it)
	   * - parent (all of it)
	   * - connections (all of it)
	   */
	  computeOneTree(
	    th,
	    currLowerBound,
	    currLowestSubgraph
	  );

	  /* better than best, and not yet a valid path */
	  bool notdone =
	    (currLowerBound < bestLowerBound) &&
	    (currLowestSubgraph > -1);

	  /* cut early, binary parallel sum */
	  reducethread(edgesimds, notdone, {
	    /* pad binary reduction "overflow" */
	    cv[length] = 0;
	  }, {
	    /* excess connections are neutral "2" */
	    uint4 d0 = (connections[i0] - 2);
	    uint4 d1 = (connections[i1] - 2);

	    cv[i] = d0 * d0 + d1 * d1;
	  }, {
	    cv[i] =  cv[i0] +  cv[i1];
	  });

	  if (notdone) {
	    if (!(currLowerBound < prevLowerBound))
	      currLambda *= (sumtype)lambda_reduction;

	    /* if zero all are == 2 */
	    uint denom = cv[0].x + cv[0].y + cv[0].z + cv[0].w;
	    if (denom != 0) {
	      /* readjust weights */
	      sumvectype t = currLambda * currLowerBound / denom;
	      limitedthread(j, edgesimds, {
	        weights[j] = weights[j] + t * sumvectype(connections[j] - 2);
	      });

	      /* exclude first (TIread "0" wrote the first, so no race condition) */
              if (firstthread(0)) {
	        weights[0][0] = 0;
	      }
	    }
          }
	}

        /* write out persistant GPU-data */
        if (firstthread(0)) {
	  persistant[0].currLambda         = currLambda;
	  persistant[0].currLambdaCount    = currLambdaCount + (/*(currLowerBound < bestLowerBound) &&*/ (currLowestSubgraph > -1) ? 1 : 0);
	  persistant[0].currLowerBound     = currLowerBound;
	  persistant[0].currLowestSubgraph = currLowestSubgraph;
        }
}
