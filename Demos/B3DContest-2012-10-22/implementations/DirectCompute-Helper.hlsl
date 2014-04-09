/* -----------------------------------------------------------------------------

	Copyright (c) 2012 Niels Fröhling              niels@paradice-insight.us

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   -------------------------------------------------------------------------- */

#ifndef	SOLVETSP_DCHELPER_HLSH
#define SOLVETSP_DCHELPER_HLSH

/* if there are problems with the results it's most likely
 * the synchronization isn't perfect still, enable "4" and
 * see if the results are still problematic
 */
//efine	WAVEFRONT_SIZE	4	// Warp
//efine	WAVEFRONT_SIZE	32	// nVidia
#define	WAVEFRONT_SIZE	64	// AMD

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/* round-to-nearest, round half towards minus infinity */
float round(float x) {
  float integer = (float)(int)x;
  float fraction = x - integer;

  return integer + (fraction <= 0.5 ? 0 : 1);
}

/* round-to-nearest, round half towards minus infinity */
double round(double x) {
  double integer = (double)(int)x;
  double fraction = x - integer;

  return integer + (fraction <= 0.5 ? 0 : 1);
}

/* horizontal add */
int hadd(int4 v) {
  int4 d = v;
  d.xz += d.yw;
  d.x  += d.z;
  return d.x;
}

/* horizontal multiply */
int hmul(int4 v) {
  int4 d = v;
  d.xz *= d.yw;
  d.x  *= d.z;
  return d.x;
}

/* horizontal multiply */
int2 hmul(int4 v0, int4 v1) {
  int4 dh;
  dh.xz = v0.xz * v0.yw;
  dh.yw = v1.xz * v1.yw;
  dh.xy = dh.xy * dh.zw;
  return dh.xy;
}

/* --------------------------------------------------------------------------
 */

sumvectype setenabled(const sumvectype left, const idxvectype right, const sumvectype mv) {
  return sumvectype(
    right.x ? left.x : mv.x,
    right.y ? left.y : mv.y,
    right.z ? left.z : mv.z,
    right.w ? left.w : mv.w
  );
}

sumvectype setdisabled(const sumvectype left, const idxvectype right, const sumvectype mv) {
  return sumvectype(
    right.x ? mv.x : left.x,
    right.y ? mv.y : left.y,
    right.z ? mv.z : left.z,
    right.w ? mv.w : left.w
  );
}

sumvectype smallers(const sumvectype left, const sumvectype right, const sumvectype assignleft, const sumvectype assignright) {
  return sumvectype(
    left.x < right.x ? assignleft.x : assignright.x,
    left.y < right.y ? assignleft.y : assignright.y,
    left.z < right.z ? assignleft.z : assignright.z,
    left.w < right.w ? assignleft.w : assignright.w
  );
}

idxvectype smalleri(const sumvectype left, const sumvectype right, const idxvectype assignleft, const idxvectype assignright) {
  return idxvectype(
    left.x < right.x ? assignleft.x : assignright.x,
    left.y < right.y ? assignleft.y : assignright.y,
    left.z < right.z ? assignleft.z : assignright.z,
    left.w < right.w ? assignleft.w : assignright.w
  );
}

idxvectype smallerii(const idxvectype left, const idxvectype right, const idxvectype assignleft, const idxvectype assignright) {
  return idxvectype(
    left.x < right.x ? assignleft.x : assignright.x,
    left.y < right.y ? assignleft.y : assignright.y,
    left.z < right.z ? assignleft.z : assignright.z,
    left.w < right.w ? assignleft.w : assignright.w
  );
}

sumvectype greaters(const sumvectype left, const sumvectype right, const sumvectype assignleft, const sumvectype assignright) {
  return sumvectype(
    left.x > right.x ? assignleft.x : assignright.x,
    left.y > right.y ? assignleft.y : assignright.y,
    left.z > right.z ? assignleft.z : assignright.z,
    left.w > right.w ? assignleft.w : assignright.w
  );
}

idxvectype greateri(const sumvectype left, const sumvectype right, const idxvectype assignleft, const idxvectype assignright) {
  return idxvectype(
    left.x > right.x ? assignleft.x : assignright.x,
    left.y > right.y ? assignleft.y : assignright.y,
    left.z > right.z ? assignleft.z : assignright.z,
    left.w > right.w ? assignleft.w : assignright.w
  );
}

idxvectype greaterii(const idxvectype left, const idxvectype right, const idxvectype assignleft, const idxvectype assignright) {
  return idxvectype(
    left.x > right.x ? assignleft.x : assignright.x,
    left.y > right.y ? assignleft.y : assignright.y,
    left.z > right.z ? assignleft.z : assignright.z,
    left.w > right.w ? assignleft.w : assignright.w
  );
}

/* --------------------------------------------------------------------------
 */

#ifndef	ROW0_REDUCTION
  /* divide into 4/32/64 buckets (wavefront size) */
  static const int blcksize = ((tilestride + WAVEFRONT_SIZE - 1) / WAVEFRONT_SIZE);

  groupshared idxvectype firstN[WAVEFRONT_SIZE];
  groupshared idxvectype secndN[WAVEFRONT_SIZE];
  groupshared sumvectype firstVal[WAVEFRONT_SIZE];
  groupshared sumvectype secndVal[WAVEFRONT_SIZE];

  void init(int w, sumvectype def) {
    firstN[w] = 0;
    secndN[w] = 0;
    firstVal[w] = def;
    secndVal[w] = def;
  }

  void orderedset(uint w,
    const sumvectype wght0, const idxvectype curindex0,
    const sumvectype wght1, const idxvectype curindex1
  ) {
    secndN  [w] = smalleri(wght0, wght1, curindex1, curindex0);
    secndVal[w] = smallers(wght0, wght1,     wght1,     wght0);

    firstN  [w] = smalleri(wght0, wght1, curindex0, curindex1);
    firstVal[w] = smallers(wght0, wght1,     wght0,     wght1);
  }

  void orderedmin(int w,
    const sumvectype wght , const idxvectype curindex
  ) {
    secndN  [w] = smalleri(wght, secndVal[w],    curindex, secndN  [w]);
    secndVal[w] = smallers(wght, secndVal[w],        wght, secndVal[w]);

    secndN  [w] = smalleri(wght, firstVal[w], firstN  [w], secndN  [w]);
    secndVal[w] = smallers(wght, firstVal[w], firstVal[w], secndVal[w]);

    firstN  [w] = smalleri(wght, firstVal[w],    curindex, firstN  [w]);
    firstVal[w] = smallers(wght, firstVal[w],        wght, firstVal[w]);
  }

  void orderedred2(int limit, int w) {
    int l = min(WAVEFRONT_SIZE, limit);
    for (int j = 2; j < l; j += 2) {
      /* assign to bucket 0/1 */
      orderedmin(w, secndVal[w + j], secndN[w + j]);
      orderedmin(w, firstVal[w + j], firstN[w + j]);
    }
  }

  void orderedred1(int limit) {
    if (1 < WAVEFRONT_SIZE) {
      /* read from bucket 1, assign to bucket 0 */
      orderedmin(0, secndVal[0 + 1], secndN[0 + 1]);
      orderedmin(0, firstVal[0 + 1], firstN[0 + 1]);
    }
  }
#else
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

/* --------------------------------------------------------------------------
 */

int usmallest(const idxvectype index, const sumvectype value) {
  idxvectype lg;
  sumvectype cg;

#if 0
  /* don't maintain lower index in case of equality */
  lg.x = (value.x > value.y ? index.y : index.x);
  lg.y = (value.z > value.w ? index.w : index.z);
  cg.x = (value.x > value.y ? value.y : value.x);
  cg.y = (value.z > value.w ? value.w : value.z);
  lg.x = (cg.x > cg.y ? lg.y : lg.x);
#else
  /* maintain lower index in case of equality */
  lg.x = (value.x >= value.y ? (value.x == value.y ? (index.y < index.x ? index.y : index.x) : index.y) : index.x);
  cg.x = (value.x >= value.y ?                                                                 value.y  : value.x);
  lg.y = (value.z >= value.w ? (value.z == value.w ? (index.w < index.z ? index.w : index.z) : index.w) : index.z);
  cg.y = (value.z >= value.w ?                                                                 value.w  : value.z);
  lg.x = (cg.x >= cg.y ? (cg.x == cg.y ? (lg.y < lg.x ? lg.y : lg.x) : lg.y) : lg.x);
#endif

  return lg.x;
}

int usmallestneg(const idxvectype index, const sumvectype value) {
  idxvectype lg;
  sumvectype cg;

#if 0
  /* don't maintain lower index in case of equality */
  lg.x = (value.x > value.y ? index.y : index.x);
  lg.z = (value.z > value.w ? index.w : index.z);
  cg.x = (value.x > value.y ? value.y : value.x);
  cg.z = (value.z > value.w ? value.w : value.z);
  lg.x = (cg.x > cg.z ? lg.z : lg.x);
  cg.x = (cg.x > cg.z ? cg.z : cg.x);
  lg.x = (cg.x > 0    ?   -1 : lg.x);
#else
  /* maintain lower index in case of equality */
  lg.x = (value.x >= value.y ? (value.x == value.y ? (index.y < index.x ? index.y : index.x) : index.y) : index.x);
  cg.x = (value.x >= value.y ?                                                                 value.y  : value.x);
  lg.z = (value.z >= value.w ? (value.z == value.w ? (index.w < index.z ? index.w : index.z) : index.w) : index.z);
  cg.z = (value.z >= value.w ?                                                                 value.w  : value.z);
  lg.x = (cg.x >= cg.z ? (cg.x == cg.z ? (lg.z < lg.x ? lg.z : lg.x) : lg.z) : lg.x);
  cg.x = (cg.x >= cg.z ?                                               cg.z  : cg.x);
  lg.x = (cg.x > 0    ?   -1 : lg.x);
#endif

  return lg.x;
}

#endif
