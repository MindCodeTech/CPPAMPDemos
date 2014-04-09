
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

#define	ROW0_REDUCTION
#include "DirectCompute-Helper.hlsl"

struct gpuio {
      sumtype currLambda;
      uint currLambdaCount;
      sumtype currLowerBound;
      int currLowestSubgraph;
};

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

RWStructuredBuffer<gpuio> persistant;

Texture2D<uint4> edgematrix;
Texture2D<uint> included;

#define	CACHEABLE
#ifndef	CACHEABLE
RWStructuredBuffer<sumvectype> wghtmatrix;
RWStructuredBuffer<sumvectype> weights;
RWStructuredBuffer<sumvectype> mstree;
RWStructuredBuffer<idxvectype> connections;
RWStructuredBuffer<idxvectype> parent;
#else
RWStructuredBuffer<sumvectype> wghtmatrix;
groupshared sumvectype weights[tilestride];	// 512*4*4->8192
groupshared sumvectype mstree[tilestride];	// 512*4*4->8192
groupshared idxvectype connections[tilestride];	// 512*4*4->8192
RWStructuredBuffer<sumvectype> weights_buf;
RWStructuredBuffer<idxvectype> parent_buf;
#endif

RWTexture1D<uint4> parents;

groupshared uint4      cv[tilestride >> 1];	// 256*4*4->4096
groupshared sumvectype mv[tilestride >> 1];	// 256*4*4->4096

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
#define reducethread(max, cond, init, seed, reduce) {		\
  /* stride always >= 8, no need to add more conditionals */	\
  uint stride = (tilestride) >> 1;				\
  /* check for pow2 */						\
  if (!(stride & (stride - 1))) {				\
    uint length = (max);					\
    								\
    /* memory isn't pow2 */					\
    {								\
      length = (length + 1) >> 1;				\
    								\
      if (((tileheight == 1) || !th.y) && (cond)) {		\
        if ((th.x < length))					\
          seed							\
        if ((th.x >= length) && (th.x < stride))		\
          init							\
      }								\
      								\
      /* ensure "half" has been calculated */			\
      yield(th, 1);						\
    }								\
      								\
    /* stride is pow2 */					\
    while (stride > 1) {					\
      stride = (stride >> 1);					\
    								\
      if (((tileheight == 1) || !th.y) && (cond)) {		\
        if ((th.x < stride))					\
	  reduce						\
      }								\
    								\
      /* ensure "quarter" has been calculated */		\
      yield(th, 2);						\
    }								\
  }								\
  /* stride isn't pow2 */					\
  else {							\
    uint length = (max);					\
    								\
    /* memory isn't pow2 */					\
    {								\
      length = (length + 1) >> 1;				\
    								\
      if (((tileheight == 1) || !th.y) && (cond)) {		\
        if ((th.x < length))					\
          seed							\
        if ((th.x == length) && (length & 1))			\
          init							\
      }								\
      								\
      /* ensure "half" has been calculated */			\
      yield(th, 1);						\
    }								\
    								\
    /* stride isn't pow2 */					\
    while (length > 1) {					\
      length = (length + 1) >> 1;				\
    								\
      if (((tileheight == 1) || !th.y) && (cond)) {		\
        if ((th.x < length))					\
	  reduce						\
        if ((th.x == length) && (length & 1))			\
          init							\
      }								\
    								\
      /* ensure "quarter" has been calculated */		\
      yield(th, 2);						\
    }								\
  }								\
    								\
  /* "broadcast" the result to all other threads		\
   * which is only possible if they stopped here		\
   * and didn't pass						\
   */								\
  yield(th, 0);							\
}

/* ********************************************************************************************
 */

//struct ampminima {
#ifndef	CACHEABLE
#else
#define	firstN		connections
#define	secndN		connections
#define	firstVal	mstree
#define	secndVal	mstree
#define	firstV		mstree
#define	secndV		mstree
#define	wghtmtxrw0	mstree

  void init(uint w, sumvectype def) {
    firstN  [(w << 1) + 0] = 0;
    secndN  [(w << 1) + 1] = 0;
    firstVal[(w << 1) + 0] = def;
    secndVal[(w << 1) + 1] = def;
  }

  void orderedset(uint w,
    const sumvectype wght0, const idxvectype curindex0,
    const sumvectype wght1, const idxvectype curindex1
  ) {
    secndN  [(w << 1) + 1] = smalleri(wght0, wght1, curindex1, curindex0);
    secndVal[(w << 1) + 1] = smallers(wght0, wght1,     wght1,     wght0);

    firstN  [(w << 1) + 0] = smalleri(wght0, wght1, curindex0, curindex1);
    firstVal[(w << 1) + 0] = smallers(wght0, wght1,     wght0,     wght1);
  }

  void orderedmin(uint w,
    const sumvectype wght, const idxvectype curindex
  ) {
    secndN  [(w << 1) + 1] = smalleri(wght, secndVal[(w << 1) + 1],               curindex, secndN  [(w << 1) + 1]);
    secndVal[(w << 1) + 1] = smallers(wght, secndVal[(w << 1) + 1],                   wght, secndVal[(w << 1) + 1]);

    secndN  [(w << 1) + 1] = smalleri(wght, firstVal[(w << 1) + 1], firstN  [(w << 1) + 0], secndN  [(w << 1) + 1]);
    secndVal[(w << 1) + 1] = smallers(wght, firstVal[(w << 1) + 1], firstVal[(w << 1) + 0], secndVal[(w << 1) + 1]);

    firstN  [(w << 1) + 0] = smalleri(wght, firstVal[(w << 1) + 0],               curindex, firstN  [(w << 1) + 0]);
    firstVal[(w << 1) + 0] = smallers(wght, firstVal[(w << 1) + 0],                   wght, firstVal[(w << 1) + 0]);
  }
#endif

  void orderedmallest(out uint firstNeighbor, out uint secndNeighbor) {
    /* get the two cheapest edges from row 0 */
    firstNeighbor = firstN[0].x;
    secndNeighbor = secndN[0].x;

    /* prevent writing back to tile_shared */
    sumtype fV_l_ = firstVal[0].x;
    sumtype sV_l_ = secndVal[0].x;

    /* reduction */
    {
      /* NF NS
	*       OF OS
	*/
      if (fV_l_ > secndVal[0].y) {
	firstNeighbor = firstN[0].y;
	secndNeighbor = secndN[0].y;

	fV_l_ = firstVal[0].y;
	sV_l_ = secndVal[0].y;
      }
      /*       NF NS
	* OF OS
	*/
      else if (sV_l_ > firstVal[0].y) {
	/* NF    NS     | NF       NS
	  *    OF    OS  |    OF OS
	  */
	if (fV_l_ > firstVal[0].y) {
	  sV_l_ = fV_l_;
	  secndNeighbor = firstNeighbor;

	  fV_l_ = firstVal[0].y;
	  firstNeighbor = firstN[0].y;
	}

	/*    NF NS     |    NF    NS
	  * OF       OS  | OF    OS
	  */
	else {
	  sV_l_ = firstVal[0].y;
	  secndNeighbor = firstN[0].y;
	}
      }
    }

    /* reduction */
    {
      /* NF NS
	*       OF OS
	*/
      if (fV_l_ > secndVal[0].z) {
	firstNeighbor = firstN[0].z;
	secndNeighbor = secndN[0].z;

	fV_l_ = firstVal[0].z;
	sV_l_ = secndVal[0].z;
      }
      /*       NF NS
	* OF OS
	*/
      else if (sV_l_ > firstVal[0].z) {
	/* NF    NS     | NF       NS
	  *    OF    OS  |    OF OS
	  */
	if (fV_l_ > firstVal[0].z) {
	  sV_l_ = fV_l_;
	  secndNeighbor = firstNeighbor;

	  fV_l_ = firstVal[0].z;
	  firstNeighbor = firstN[0].z;
	}

	/*    NF NS     |    NF    NS
	  * OF       OS  | OF    OS
	  */
	else {
	  sV_l_ = firstVal[0].z;
	  secndNeighbor = firstN[0].z;
	}
      }
    }

    /* reduction */
    {
      /* NF NS
	*       OF OS
	*/
      if (fV_l_ > secndVal[0].w) {
	firstNeighbor = firstN[0].w;
	secndNeighbor = secndN[0].w;

	fV_l_ = firstVal[0].w;
	sV_l_ = secndVal[0].w;
      }
      /*       NF NS
	* OF OS
	*/
      else if (sV_l_ > firstVal[0].w) {
	/* NF    NS     | NF       NS
	  *    OF    OS  |    OF OS
	  */
	if (fV_l_ > firstVal[0].w) {
	  sV_l_ = fV_l_;
	  secndNeighbor = firstNeighbor;

	  fV_l_ = firstVal[0].w;
	  firstNeighbor = firstN[0].w;
	}

	/*    NF NS     |    NF    NS
	  * OF       OS  | OF    OS
	  */
	else {
	  sV_l_ = firstVal[0].w;
	  secndNeighbor = firstN[0].w;
	}
      }
    }
  }
//};

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

  void initialize0(uint3 th, bool init) {
    /* mask out over-aligned weights */
    sumvectype l = sumvectype(
      (3 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0),
      (2 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0),
      (1 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0),
      (0 < (SIMDsize - numnodes % SIMDsize) ? maxvec : 0)
    );

    /* if "numnodes % SIMDsize" == 0, then "numnodes / SIMDsize" == edgesimds (< j) */
    limitedthread(j, (edgesimds), {
      if (init)
        weights[j] = (j == (numnodes / SIMDsize) ? l : 0);
      else
        weights[j] = weights_buf[j];
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

  void initialize2(uint3 th, inout uint4 parent_j_, uint firstNeighbor) {
    /* mask out over-aligned connections */
    idxvectype l = idxvectype(
      (3 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0),
      (2 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0),
      (1 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0),
      (0 < (SIMDsize - numnodes % SIMDsize) ? 2 : 0)
    );

    /* if "numnodes % SIMDsize" == 0, then "numnodes / SIMDsize" == edgesimds (< j) */
    limitedthread(j, edgesimds, {
      connections[j] = (j == (numnodes / SIMDsize) ? l : 0);
      parent_j_ = firstNeighbor;
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
    inout idxvectype parent_j_,
    inout sumtype lowerBound,
    inout int lowestSubgraph
  ) {
    /* globally valid thread-to-location mappings */
    const uint r  = (th.x << 0) + 0;
    const uint r0 = (th.x << 1) + 0;
    const uint r1 = (th.x << 1) + 1;

    const uint pos  = (r  * SIMDsize);
    const uint pos0 = (r0 * SIMDsize);
    const uint pos1 = (r1 * SIMDsize);

    const uint4 curindex  = uint4(pos  + 0, pos  + 1, pos  + 2, pos  + 3);
    const uint4 curindex0 = uint4(pos0 + 0, pos0 + 1, pos0 + 2, pos0 + 3);
    const uint4 curindex1 = uint4(pos1 + 0, pos1 + 1, pos1 + 2, pos1 + 3);

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

	uint cpos = (col * SIMDsize);
	uint basebit = cpos & 31;
	uint bitfield = included[uint2(row, cpos >> 5)];

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
#ifdef	CACHEABLE
	wghtmtxrw0[col] = wght;
#endif
      });
    });

    /* ensure "wghtmatrix[0]" has been calculated */
    yield(th);

    /* get the two cheapest edges from 0 --------------------------------------------------- */
    uint firstNeighbor  = 0;
    uint secondNeighbor = 0;

#ifndef	CACHEABLE
    /* horizontal reduction (X to 4/32/64, vectorized, single wave-front) */
    wfrontthread(w, {
      init(w, maxvec);

      /* start & end, clamped to edgestride */
      uint l = min((w + 0) * blcksize, edgesimds);
      uint h = min((w + 1) * blcksize, edgesimds);
      for (uint col = l; col < h; col++) {
        uint cpos = (col * SIMDsize);
        uint4 curindex = uint4(cpos + 0, cpos + 1, cpos + 2, cpos + 3);
        sumvectype wght = wghtmatrix[0 * edgestride + col];

        /* assign to one of the 4/32/64 buckets */
        orderedmin(w, wght, curindex);
      }
    });

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
#else
    reducethread(edgesimds, 1, {
      /* pad binary reduction "overflow" */
      init(r, maxvec);
    }, {
      sumvectype wght0 = wghtmtxrw0[r0];
      sumvectype wght1 = wghtmtxrw0[r1];

      /* put the smaller in "first", the larger in "second" */
      orderedset(r, wght0, curindex0,
      		    wght1, curindex1);
    }, {
      /* the i0's "second" may overlap i's "second" */
      uint4 index0 = secndN[(r0 << 1) + 1];
      uint4 index1 = secndN[(r1 << 1) + 1];
      sumvectype wght0 = secndV[(r0 << 1) + 1];
      sumvectype wght1 = secndV[(r1 << 1) + 1];

      /* put the smaller in "first", the larger in "second" */
      orderedset(r, firstV[(r0 << 1) + 0], firstN[(r0 << 1) + 0],
      		    firstV[(r1 << 1) + 0], firstN[(r1 << 1) + 0]);

      /* check how the remaining values fit in */
      orderedmin(r, wght0, index0);
      orderedmin(r, wght1, index1);
    });
#endif

    /* horizontal reduction (4-way vector to scalar, suck from thread-local into registers) */
    orderedmallest(firstNeighbor, secondNeighbor);

    /* initialize tree */
    initialize2(th, parent_j_, firstNeighbor);

    /* compute the minimum spanning tree on nodes [1, numnodes - 1] ------------------------ */
    lowerBound = 0;

    /* copy out initial costs from weight-matrix (waits for "wghtmatrix" being valid) */
    initialize1(th, firstNeighbor);

    /* select the thread responsible for parent[firstNeighbor / SIMDsize] */
    singlethread(firstNeighbor / SIMDsize, {
      /* add first edge */
      connect(0, firstNeighbor, lowerBound);
      parent_j_ = uint4(
        ((firstNeighbor % SIMDsize) == 0) ? 0 : parent_j_.x,
        ((firstNeighbor % SIMDsize) == 1) ? 0 : parent_j_.y,
        ((firstNeighbor % SIMDsize) == 2) ? 0 : parent_j_.z,
        ((firstNeighbor % SIMDsize) == 3) ? 0 : parent_j_.w
      );
    });

    /* ensure "connect[]" is visible to other threads/wavefronts */
    yield(th);

    /* consume all unassigned edges */
    for (uint k = 2, l = 0, h = (edgesimds); k < (numnodes /* * th.y */); k++) {
      /* search first unassigned with lowest cost  - - - - - - - - - - - - - - - - - - - - - */

      /* ensure "parent[]" and "mstree[]" has been calculated */
      yield(th);

#if	defined(CACHEABLE) && 0
      /* l & h is the same for all threads now */
      if (firstthread(0)) {
        hh = l;
        ll = h;
      }
#endif

      reducethread(h, 1, {
	/* pad binary reduction "overflow" */
	mv[r] = maxvec;
      }, {
	/* excess connections are neutral "2" */
	uint4 con0 = connections[r0];
	uint4 con1 = connections[r1];
	idxvectype cv_r_ = 0;
	sumvectype mv_r_ = maxvec;

	/* skip i/o if not necessary (all non-zero) */
	uint4 chk;

	chk.xy = con1.xy * con1.zw;
	chk.zw = con0.xy * con0.zw;
	chk.x  = chk.x * chk.y;
	chk.z  = chk.z * chk.w;

	if (!(chk.x * chk.z)) {
	  /* skip connections of "> 0" (assign MAX, voids the "smaller") */
	  sumvectype mstv0 = mstree[r0];
	  sumvectype mstv1 = mstree[r1];

	  mstv0 = setdisabled(mstv0, con0, cv_r_);
	  mstv1 = setdisabled(mstv1, con1, mv_r_);

	  /* maintain lower index in case of equality */
	  cv_r_ = smalleri(mstv1, mstv0, curindex1, curindex0);
	  mv_r_ = smallers(mstv1, mstv0,     mstv1,     mstv0);

#if	defined(CACHEABLE) && 0
	  /* i1 > i0 */
	  if (!chk.x) {
	    InterlockedMin(ll, r1);
	    InterlockedMax(hh, r1);
	  }

	  if (!chk.z) {
	    InterlockedMin(ll, r0);
	    InterlockedMax(hh, r0);
	  }
#endif
	}

	cv[r] = cv_r_;
	mv[r] = mv_r_;
      }, {
	cv[r] = smalleri(mv[r1], mv[r0], cv[r1], cv[r0]);
	mv[r] = smallers(mv[r1], mv[r0], mv[r1], mv[r0]);
      });

#if	defined(CACHEABLE) && 0
      l = ll + 0;
      h = hh + 1;
#endif

      /* add unassigned with lowest cost - - - - - - - - - - - - - - - - - - - - - - - - - - */
      uint i = usmallest(cv[0], mv[0]);

      singlethread(i / SIMDsize, {
        connect(parent_j_[i % SIMDsize], i, lowerBound);
      });

      /* ensure "connect[]" and "parent[]" is visible to other threads/wavefronts */
      yield(th);

      /* reassign costs  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
      uint4 iv = i;

      rangedthread(l, h, {
	/* excess connections are neutral "2" */
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
	  parent_j_ = greateri(mstv, wght, iv, parent_j_);
	  mstree[j] = greaters(mstv, wght, wght, mstv);
        }
      });
    }

    /* add last edge */
    if (firstthread(0)) {
      connect(0, secondNeighbor, lowerBound);
      parent_j_[0] = secondNeighbor;
    }

    /* ensure "connect[]" and "parent[]" is visible to other threads/wavefronts */
    broadcast(th, false /* == memory */);

    /* built shortest non-cycle sub-graph -------------------------------------------------- */
    reducethread(edgesimds, 1, {
      /* pad binary reduction "overflow" */
      mv[r] = 0x7FFF0000;
    },{
      /* excess connections are neutral "2" */
      idxvectype con0 = (connections[r0] << 16) + 0x7FFD0000; /* -2 wraparound signflip */
      idxvectype con1 = (connections[r1] << 16) + 0x7FFD0000; /* -2 wraparound signflip */

      /* maintain lower index in case of equality */
      idxvectype ab = smallerii(con1, con0, curindex1, curindex0);
      idxvectype cd = smallerii(con1, con0,      con1,      con0);

      cv[r] = ab;
      mv[r] = cd;
    }, {
      cv[r] = smalleri(mv[r1], mv[r0], cv[r1], cv[r0]);
      mv[r] = smallers(mv[r1], mv[r0], mv[r1], mv[r0]);
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
    /* globally valid thread-to-location mappings */
    const uint r  = (th.x << 0) + 0;
    const uint r0 = (th.x << 1) + 0;
    const uint r1 = (th.x << 1) + 1;

    const uint pos  = (r  * SIMDsize);
    const uint pos0 = (r0 * SIMDsize);
    const uint pos1 = (r1 * SIMDsize);

    const uint4 curindex  = uint4(pos  + 0, pos  + 1, pos  + 2, pos  + 3);
    const uint4 curindex0 = uint4(pos0 + 0, pos0 + 1, pos0 + 2, pos0 + 3);
    const uint4 curindex1 = uint4(pos1 + 0, pos1 + 1, pos1 + 2, pos1 + 3);

	/* per-thread local 1:1 relationships */
	idxvectype pt;

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
	initialize0(th, !currLowestSubgraph);

//	while (lambda > lambda_termination)
	{
	  /* inner shader loop is on its first iteration  */
	  bool loopenter = true;
	  /* inner shader loop is on its last iteration  */
	  bool loopfinish = true;

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
	    pt,
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
	    cv[r] = 0;
	  }, {
	    /* excess connections are neutral "2" */
	    uint4 d0 = (connections[r0] - 2);
	    uint4 d1 = (connections[r1] - 2);

	    cv[r] = d0 * d0 + d1 * d1;
	  }, {
	    cv[r] =  cv[r0] +  cv[r1];
	  });

	  if (notdone) {
	    if (!(currLowerBound < prevLowerBound))
	      currLambda *= (sumtype)lambda_reduction;

	    /* if zero all are == 2 */
	    idxvectype d = cv[0];
	    d.xz = int2(d.x + d.y, d.z + d.w);
	    int denom = d.x + d.z;

	    /* readjust weights */
	    sumvectype t = currLambda * currLowerBound / denom;
	    limitedthread(j, edgesimds, {
	      sumvectype w = weights[j];

	      /* add up every except first */
	      w = w + t * sumvectype(connections[j] - 2);
	      w.x = (!j ? 0 : w.x);

	      weights[j] = w;
	      if (loopfinish)
	        weights_buf[j] = w;
	    });
          }
	}

	/* better than best, and a completely valid path */
	bool better =
	  ((currLambda <= lambda_termination) ||	// regular loop condition
	   (currLowestSubgraph == -1)) &&		// early loop termination
	   (currLowerBound < bestLowerBound);

	if (better) {
	  limitedthread(j, edgesimds, {
	    parent_buf[j] = pt;
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
