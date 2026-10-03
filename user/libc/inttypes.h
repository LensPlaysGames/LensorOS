/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _LENSOROS_LIBC_INTTYPES_H
#define _LENSOROS_LIBC_INTTYPES_H

#include <stddef.h>
#include <stdint.h>

#define PRIdN "%d"
#define PRIdLEASTN PRIdN
#define PRIdFASTN PRIdN
#define PRIdMAX PRIdN
#define PRIdPTR PRIdN

#define PRIiN "%i"
#define PRIiLEASTN PRIdN
#define PRIiFASTN PRIdN
#define PRIiMAX PRIdN
#define PRIiPTR PRIdN

#define PRIoN "%u"  // FIXME:
#define PRIoLEASTN PRIoN
#define PRIoFASTN PRIoN
#define PRIoMAX PRIoN
#define PRIoPTR PRIoN

#define PRIuN "%u"
#define PRIuLEASTN PRIuN
#define PRIuFASTN PRIuN
#define PRIuMAX PRIuN
#define PRIuPTR PRIuN

#define PRIxN "%u"  // FIXME
#define PRIxLEASTN PRIxN
#define PRIxFASTN PRIxN
#define PRIxMAX PRIxN
#define PRIxPTR PRIxN

#define PRIXN "%u"  // FIXME
#define PRIXLEASTN PRIXN
#define PRIXFASTN PRIXN
#define PRIXMAX PRIXN
#define PRIXPTR PRIXNSCNdN

#define SCNdN "%d"
#define SCNdLEASTN SCNdN
#define SCNdFASTN SCNdN
#define SCNdMAX SCNdN
#define SCNdPTR SCNdN

#define SCNiN "%i"
#define SCNiLEASTN SCNiN
#define SCNiFASTN SCNiN
#define SCNiMAX SCNiN
#define SCNiPTR SCNiN

#define SCNoN "%u"  // FIXME
#define SCNoLEASTN SCNoN
#define SCNoFASTN SCNoN
#define SCNoMAX SCNoN
#define SCNoPTR SCNoN

#define SCNuN "%u"
#define SCNuLEASTN SCNuN
#define SCNuFASTN SCNuN
#define SCNuMAX SCNuN
#define SCNuPTR SCNuN

#define SCNxN "%u"  // FIXME
#define SCNxLEASTN SCNxN
#define SCNxFASTN SCNxN
#define SCNxMAX SCNxN
#define SCNxPTR SCNxN

#endif
