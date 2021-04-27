/*
  This source is part of the libosmscout library
  Copyright (C) 2021  Jean-Luc Barriere

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/util/utf8helper_charmap.h>

namespace utf8helper
{

const character* pagemap_16[32] = {
  /* c0 */
  nullptr, nullptr, charmap_c2, charmap_c3, charmap_c4, charmap_c5, charmap_c6, charmap_c7,
  charmap_c8, charmap_c9, charmap_ca, charmap_cb, charmap_cc, charmap_cd, charmap_ce, charmap_cf,
  charmap_d0, charmap_d1, charmap_d2, charmap_d3, charmap_d4, charmap_d5, charmap_d6, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

const character charmap_us7ascii[128] = {
  { 0x00 , 0x00 , 0x00 , None , "" },
  { 0x01 , 0x01 , 0x01 , IsControl , "" },
  { 0x02 , 0x02 , 0x02 , IsControl , "" },
  { 0x03 , 0x03 , 0x03 , IsControl , "" },
  { 0x04 , 0x04 , 0x04 , IsControl , "" },
  { 0x05 , 0x05 , 0x05 , IsControl , "" },
  { 0x06 , 0x06 , 0x06 , IsControl , "" },
  { 0x07 , 0x07 , 0x07 , IsControl , "" },
  { 0x08 , 0x08 , 0x08 , IsControl , "" },
  { 0x09 , 0x09 , 0x09 , IsSpace | IsBreaker , " " },
  { 0x0a , 0x0a , 0x0a , IsBreaker , u8"\x0a" },
  { 0x0b , 0x0b , 0x0b , IsControl , "" },
  { 0x0c , 0x0c , 0x0c , IsControl , "" },
  { 0x0d , 0x0d , 0x0d , IsControl , "" },
  { 0x0e , 0x0e , 0x0e , IsControl , "" },
  { 0x0f , 0x0f , 0x0f , IsControl , "" },
  { 0x10 , 0x10 , 0x10 , IsControl , "" },
  { 0x11 , 0x11 , 0x11 , IsControl , "" },
  { 0x12 , 0x12 , 0x12 , IsControl , "" },
  { 0x13 , 0x13 , 0x13 , IsControl , "" },
  { 0x14 , 0x14 , 0x14 , IsControl , "" },
  { 0x15 , 0x15 , 0x15 , IsControl , "" },
  { 0x16 , 0x16 , 0x16 , IsControl , "" },
  { 0x17 , 0x17 , 0x17 , IsControl , "" },
  { 0x18 , 0x18 , 0x18 , IsControl , "" },
  { 0x19 , 0x19 , 0x19 , IsControl , "" },
  { 0x1a , 0x1a , 0x1a , IsControl , "" },
  { 0x1b , 0x1b , 0x1b , IsControl , "" },
  { 0x1c , 0x1c , 0x1c , IsControl , "" },
  { 0x1d , 0x1d , 0x1d , IsControl , "" },
  { 0x1e , 0x1e , 0x1e , IsControl , "" },
  { 0x1f , 0x1f , 0x1f , IsControl , "" },
  { 0x20 , 0x20 , 0x20 , IsSpace | IsBreaker , " " },
  { 0x21 , 0x21 , 0x21 , IsPunctuation , u8"\x21" }, /* ! */
  { 0x22 , 0x22 , 0x22 , IsPunctuation , u8"\x22" }, /* " */
  { 0x23 , 0x23 , 0x23 , None , u8"\x23" }, /* # */
  { 0x24 , 0x24 , 0x24 , None , u8"\x24" }, /* $ */
  { 0x25 , 0x25 , 0x25 , None , u8"\x25" }, /* % */
  { 0x26 , 0x26 , 0x26 , None , u8"\x26" }, /* & */
  { 0x27 , 0x27 , 0x27 , IsPunctuation , u8"\x27" }, /* APO */
  { 0x28 , 0x28 , 0x28 , IsPunctuation , u8"\x28" }, /* ( */
  { 0x29 , 0x29 , 0x29 , IsPunctuation , u8"\x29" }, /* ) */
  { 0x2a , 0x2a , 0x2a , None , u8"\x2a" }, /* * */
  { 0x2b , 0x2b , 0x2b , None , u8"\x2b" }, /* + */
  { 0x2c , 0x2c , 0x2c , IsPunctuation , u8"\x2c" }, /* COMMA */
  { 0x2d , 0x2d , 0x2d , IsPunctuation , u8"\x2d" }, /* - */
  { 0x2e , 0x2e , 0x2e , IsBreaker , u8"\x2e" }, /* . */
  { 0x2f , 0x2f , 0x2f , None , u8"\x2f" }, /* / */
  { 0x30 , 0x30 , 0x30 , None , u8"\x30" }, /* 0 */
  { 0x31 , 0x31 , 0x31 , None , u8"\x31" }, /* 1 */
  { 0x32 , 0x32 , 0x32 , None , u8"\x32" }, /* 2 */
  { 0x33 , 0x33 , 0x33 , None , u8"\x33" }, /* 3 */
  { 0x34 , 0x34 , 0x34 , None , u8"\x34" }, /* 4 */
  { 0x35 , 0x35 , 0x35 , None , u8"\x35" }, /* 5 */
  { 0x36 , 0x36 , 0x36 , None , u8"\x36" }, /* 6 */
  { 0x37 , 0x37 , 0x37 , None , u8"\x37" }, /* 7 */
  { 0x38 , 0x38 , 0x38 , None , u8"\x38" }, /* 8 */
  { 0x39 , 0x39 , 0x39 , None , u8"\x39" }, /* 9 */
  { 0x3a , 0x3a , 0x3a , IsPunctuation , u8"\x3a" }, /* : */
  { 0x3b , 0x3b , 0x3b , IsPunctuation , u8"\x3b" }, /* SEMICOLON */
  { 0x3c , 0x3c , 0x3c , IsPunctuation , u8"\x3c" }, /* < */
  { 0x3d , 0x3d , 0x3d , None , u8"\x3d" }, /* = */
  { 0x3e , 0x3e , 0x3e , IsPunctuation , u8"\x3e" }, /* > */
  { 0x3f , 0x3f , 0x3f , IsPunctuation , u8"\x3f" }, /* ? */
  { 0x40 , 0x40 , 0x40 , None , u8"\x40" }, /* @ */
  { 0x41 , 0x41 , 0x61 , None , u8"\x41" }, /* A : up=A : lo=a */
  { 0x42 , 0x42 , 0x62 , None , u8"\x42" }, /* B : up=B : lo=b */
  { 0x43 , 0x43 , 0x63 , None , u8"\x43" }, /* C : up=C : lo=c */
  { 0x44 , 0x44 , 0x64 , None , u8"\x44" }, /* D : up=D : lo=d */
  { 0x45 , 0x45 , 0x65 , None , u8"\x45" }, /* E : up=E : lo=e */
  { 0x46 , 0x46 , 0x66 , None , u8"\x46" }, /* F : up=F : lo=f */
  { 0x47 , 0x47 , 0x67 , None , u8"\x47" }, /* G : up=G : lo=g */
  { 0x48 , 0x48 , 0x68 , None , u8"\x48" }, /* H : up=H : lo=h */
  { 0x49 , 0x49 , 0x69 , None , u8"\x49" }, /* I : up=I : lo=i */
  { 0x4a , 0x4a , 0x6a , None , u8"\x4a" }, /* J : up=J : lo=j */
  { 0x4b , 0x4b , 0x6b , None , u8"\x4b" }, /* K : up=K : lo=k */
  { 0x4c , 0x4c , 0x6c , None , u8"\x4c" }, /* L : up=L : lo=l */
  { 0x4d , 0x4d , 0x6d , None , u8"\x4d" }, /* M : up=M : lo=m */
  { 0x4e , 0x4e , 0x6e , None , u8"\x4e" }, /* N : up=N : lo=n */
  { 0x4f , 0x4f , 0x6f , None , u8"\x4f" }, /* O : up=O : lo=o */
  { 0x50 , 0x50 , 0x70 , None , u8"\x50" }, /* P : up=P : lo=p */
  { 0x51 , 0x51 , 0x71 , None , u8"\x51" }, /* Q : up=Q : lo=q */
  { 0x52 , 0x52 , 0x72 , None , u8"\x52" }, /* R : up=R : lo=r */
  { 0x53 , 0x53 , 0x73 , None , u8"\x53" }, /* S : up=S : lo=s */
  { 0x54 , 0x54 , 0x74 , None , u8"\x54" }, /* T : up=T : lo=t */
  { 0x55 , 0x55 , 0x75 , None , u8"\x55" }, /* U : up=U : lo=u */
  { 0x56 , 0x56 , 0x76 , None , u8"\x56" }, /* V : up=V : lo=v */
  { 0x57 , 0x57 , 0x77 , None , u8"\x57" }, /* W : up=W : lo=w */
  { 0x58 , 0x58 , 0x78 , None , u8"\x58" }, /* X : up=X : lo=x */
  { 0x59 , 0x59 , 0x79 , None , u8"\x59" }, /* Y : up=Y : lo=y */
  { 0x5a , 0x5a , 0x7a , None , u8"\x5a" }, /* Z : up=Z : lo=z */
  { 0x5b , 0x5b , 0x5b , IsPunctuation , u8"\x5b" }, /* [ */
  { 0x5c , 0x5c , 0x5c , None , u8"\x5c" }, /* BACKSLASH */
  { 0x5d , 0x5d , 0x5d , IsPunctuation , u8"\x5d" }, /* ] */
  { 0x5e , 0x5e , 0x5e , IsDiacritic , u8"\x5e" }, /* ^ */
  { 0x5f , 0x5f , 0x5f , None , u8"\x5f" }, /* _ */
  { 0x60 , 0x60 , 0x60 , IsDiacritic , u8"\x60" }, /* ` */
  { 0x61 , 0x41 , 0x61 , None , u8"\x61" }, /* a : up=A : lo=a */
  { 0x62 , 0x42 , 0x62 , None , u8"\x62" }, /* b : up=B : lo=b */
  { 0x63 , 0x43 , 0x63 , None , u8"\x63" }, /* c : up=C : lo=c */
  { 0x64 , 0x44 , 0x64 , None , u8"\x64" }, /* d : up=D : lo=d */
  { 0x65 , 0x45 , 0x65 , None , u8"\x65" }, /* e : up=E : lo=e */
  { 0x66 , 0x46 , 0x66 , None , u8"\x66" }, /* f : up=F : lo=f */
  { 0x67 , 0x47 , 0x67 , None , u8"\x67" }, /* g : up=G : lo=g */
  { 0x68 , 0x48 , 0x68 , None , u8"\x68" }, /* h : up=H : lo=h */
  { 0x69 , 0x49 , 0x69 , None , u8"\x69" }, /* i : up=I : lo=i */
  { 0x6a , 0x4a , 0x6a , None , u8"\x6a" }, /* j : up=J : lo=j */
  { 0x6b , 0x4b , 0x6b , None , u8"\x6b" }, /* k : up=K : lo=k */
  { 0x6c , 0x4c , 0x6c , None , u8"\x6c" }, /* l : up=L : lo=l */
  { 0x6d , 0x4d , 0x6d , None , u8"\x6d" }, /* m : up=M : lo=m */
  { 0x6e , 0x4e , 0x6e , None , u8"\x6e" }, /* n : up=N : lo=n */
  { 0x6f , 0x4f , 0x6f , None , u8"\x6f" }, /* o : up=O : lo=o */
  { 0x70 , 0x50 , 0x70 , None , u8"\x70" }, /* p : up=P : lo=p */
  { 0x71 , 0x51 , 0x71 , None , u8"\x71" }, /* q : up=Q : lo=q */
  { 0x72 , 0x52 , 0x72 , None , u8"\x72" }, /* r : up=R : lo=r */
  { 0x73 , 0x53 , 0x73 , None , u8"\x73" }, /* s : up=S : lo=s */
  { 0x74 , 0x54 , 0x74 , None , u8"\x74" }, /* t : up=T : lo=t */
  { 0x75 , 0x55 , 0x75 , None , u8"\x75" }, /* u : up=U : lo=u */
  { 0x76 , 0x56 , 0x76 , None , u8"\x76" }, /* v : up=V : lo=v */
  { 0x77 , 0x57 , 0x77 , None , u8"\x77" }, /* w : up=W : lo=w */
  { 0x78 , 0x58 , 0x78 , None , u8"\x78" }, /* x : up=X : lo=x */
  { 0x79 , 0x59 , 0x79 , None , u8"\x79" }, /* y : up=Y : lo=y */
  { 0x7a , 0x5a , 0x7a , None , u8"\x7a" }, /* z : up=Z : lo=z */
  { 0x7b , 0x7b , 0x7b , IsPunctuation , u8"\x7b" }, /* { */
  { 0x7c , 0x7c , 0x7c , IsBreaker , u8"\x7c" }, /* | */
  { 0x7d , 0x7d , 0x7d , IsPunctuation , u8"\x7d" }, /* } */
  { 0x7e , 0x7e , 0x7e , IsPunctuation , u8"\x7e" }, /* ~ */
  { 0x7f , 0x7f , 0x7f , IsControl , "" },
};

const character charmap_c2[64] = {
  { 0xc280 , 0xc280 , 0xc280 , IsControl , "" },
  { 0xc281 , 0xc281 , 0xc281 , IsControl , "" },
  { 0xc282 , 0xc282 , 0xc282 , IsControl , "" },
  { 0xc283 , 0xc283 , 0xc283 , IsControl , "" },
  { 0xc284 , 0xc284 , 0xc284 , IsControl , "" },
  { 0xc285 , 0xc285 , 0xc285 , IsControl , "" },
  { 0xc286 , 0xc286 , 0xc286 , IsControl , "" },
  { 0xc287 , 0xc287 , 0xc287 , IsControl , "" },
  { 0xc288 , 0xc288 , 0xc288 , IsControl , "" },
  { 0xc289 , 0xc289 , 0xc289 , IsControl , "" },
  { 0xc28a , 0xc28a , 0xc28a , IsControl , "" },
  { 0xc28b , 0xc28b , 0xc28b , IsControl , "" },
  { 0xc28c , 0xc28c , 0xc28c , IsControl , "" },
  { 0xc28d , 0xc28d , 0xc28d , IsControl , "" },
  { 0xc28e , 0xc28e , 0xc28e , IsControl , "" },
  { 0xc28f , 0xc28f , 0xc28f , IsControl , "" },
  { 0xc290 , 0xc290 , 0xc290 , IsControl , "" },
  { 0xc291 , 0xc291 , 0xc291 , IsControl , "" },
  { 0xc292 , 0xc292 , 0xc292 , IsControl , "" },
  { 0xc293 , 0xc293 , 0xc293 , IsControl , "" },
  { 0xc294 , 0xc294 , 0xc294 , IsControl , "" },
  { 0xc295 , 0xc295 , 0xc295 , IsControl , "" },
  { 0xc296 , 0xc296 , 0xc296 , IsControl , "" },
  { 0xc297 , 0xc297 , 0xc297 , IsControl , "" },
  { 0xc298 , 0xc298 , 0xc298 , IsControl , "" },
  { 0xc299 , 0xc299 , 0xc299 , IsControl , "" },
  { 0xc29a , 0xc29a , 0xc29a , IsControl , "" },
  { 0xc29b , 0xc29b , 0xc29b , IsControl , "" },
  { 0xc29c , 0xc29c , 0xc29c , IsControl , "" },
  { 0xc29d , 0xc29d , 0xc29d , IsControl , "" },
  { 0xc29e , 0xc29e , 0xc29e , IsControl , "" },
  { 0xc29f , 0xc29f , 0xc29f , IsControl , "" },
  { 0xc2a0 , 0xc2a0 , 0xc2a0 , IsSpace , " " },
  { 0xc2a1 , 0xc2a1 , 0xc2a1 , None , "!" }, /* ¡ : up=¡ : lo=¡ */
  { 0xc2a2 , 0xc2a2 , 0xc2a2 , None , "c" }, /* ¢ : up=¢ : lo=¢ */
  { 0xc2a3 , 0xc2a3 , 0xc2a3 , None , "GBP" }, /* £ : up=£ : lo=£ */
  { 0xc2a4 , 0xc2a4 , 0xc2a4 , None , "" }, /* ¤ : up=¤ : lo=¤ */
  { 0xc2a5 , 0xc2a5 , 0xc2a5 , None , "JPY" }, /* ¥ : up=¥ : lo=¥ */
  { 0xc2a6 , 0xc2a6 , 0xc2a6 , IsBreaker , "|" }, /* ¦ : up=¦ : lo=¦ */
  { 0xc2a7 , 0xc2a7 , 0xc2a7 , None , "" }, /* § : up=§ : lo=§ */
  { 0xc2a8 , 0xc2a8 , 0xc2a8 , None , "" }, /* ¨ : up=¨ : lo=¨ */
  { 0xc2a9 , 0xc2a9 , 0xc2a9 , None , "(C)" }, /* © : up=© : lo=© */
  { 0xc2aa , 0xc2aa , 0xc2aa , None , "a" }, /* ª : up=ª : lo=ª */
  { 0xc2ab , 0xc2ab , 0xc2ab , IsPunctuation , "\"" }, /* « : up=« : lo=« */
  { 0xc2ac , 0xc2ac , 0xc2ac , None , "!" }, /* ¬ : up=¬ : lo=¬ */
  { 0xc2ad , 0xc2ad , 0xc2ad , IsControl , "" }, /* Conditional hyphen (SHY) */
  { 0xc2ae , 0xc2ae , 0xc2ae , None , "(R)" }, /* ® : up=® : lo=® */
  { 0xc2af , 0xc2af , 0xc2af , None , "" }, /* ¯ : up=¯ : lo=¯ */
  { 0xc2b0 , 0xc2b0 , 0xc2b0 , None , "" }, /* ° : up=° : lo=° */
  { 0xc2b1 , 0xc2b1 , 0xc2b1 , None , "+-" }, /* ± : up=± : lo=± */
  { 0xc2b2 , 0xc2b2 , 0xc2b2 , None , "2" }, /* ² : up=² : lo=² */
  { 0xc2b3 , 0xc2b3 , 0xc2b3 , None , "3" }, /* ³ : up=³ : lo=³ */
  { 0xc2b4 , 0xc2b4 , 0xc2b4 , IsDiacritic , u8"\x27" }, /* ´ : up=´ : lo=´ */
  { 0xc2b5 , 0xce9c , 0xc2b5 , None , "u" }, /* µ : up=Μ : lo=µ */
  { 0xc2b6 , 0xc2b6 , 0xc2b6 , None , "" }, /* ¶ : up=¶ : lo=¶ */
  { 0xc2b7 , 0xc2b7 , 0xc2b7 , IsBreaker , "." }, /* · : up=· : lo=· */
  { 0xc2b8 , 0xc2b8 , 0xc2b8 , IsPunctuation , u8"\x2c" }, /* ¸ : up=¸ : lo=¸ */
  { 0xc2b9 , 0xc2b9 , 0xc2b9 , None , "1" }, /* ¹ : up=¹ : lo=¹ */
  { 0xc2ba , 0xc2ba , 0xc2ba , None , "o" }, /* º : up=º : lo=º */
  { 0xc2bb , 0xc2bb , 0xc2bb , IsPunctuation , "\"" }, /* » : up=» : lo=» */
  { 0xc2bc , 0xc2bc , 0xc2bc , None , "1/4" }, /* ¼ : up=¼ : lo=¼ */
  { 0xc2bd , 0xc2bd , 0xc2bd , None , "1/2" }, /* ½ : up=½ : lo=½ */
  { 0xc2be , 0xc2be , 0xc2be , None , "3/4" }, /* ¾ : up=¾ : lo=¾ */
  { 0xc2bf , 0xc2bf , 0xc2bf , IsPunctuation , "?" }, /* ¿ : up=¿ : lo=¿ */
};

const character charmap_c3[64] = {
  { 0xc380 , 0xc380 , 0xc3a0 , None , "A" }, /* À : up=À : lo=à */
  { 0xc381 , 0xc381 , 0xc3a1 , None , "A" }, /* Á : up=Á : lo=á */
  { 0xc382 , 0xc382 , 0xc3a2 , None , "A" }, /* Â : up=Â : lo=â */
  { 0xc383 , 0xc383 , 0xc3a3 , None , "A" }, /* Ã : up=Ã : lo=ã */
  { 0xc384 , 0xc384 , 0xc3a4 , None , "A" }, /* Ä : up=Ä : lo=ä */
  { 0xc385 , 0xc385 , 0xc3a5 , None , "A" }, /* Å : up=Å : lo=å */
  { 0xc386 , 0xc386 , 0xc3a6 , None , "AE" }, /* Æ : up=Æ : lo=æ */
  { 0xc387 , 0xc387 , 0xc3a7 , None , "C" }, /* Ç : up=Ç : lo=ç */
  { 0xc388 , 0xc388 , 0xc3a8 , None , "E" }, /* È : up=È : lo=è */
  { 0xc389 , 0xc389 , 0xc3a9 , None , "E" }, /* É : up=É : lo=é */
  { 0xc38a , 0xc38a , 0xc3aa , None , "E" }, /* Ê : up=Ê : lo=ê */
  { 0xc38b , 0xc38b , 0xc3ab , None , "E" }, /* Ë : up=Ë : lo=ë */
  { 0xc38c , 0xc38c , 0xc3ac , None , "I" }, /* Ì : up=Ì : lo=ì */
  { 0xc38d , 0xc38d , 0xc3ad , None , "I" }, /* Í : up=Í : lo=í */
  { 0xc38e , 0xc38e , 0xc3ae , None , "I" }, /* Î : up=Î : lo=î */
  { 0xc38f , 0xc38f , 0xc3af , None , "I" }, /* Ï : up=Ï : lo=ï */
  { 0xc390 , 0xc390 , 0xc3b0 , None , "D" }, /* Ð : up=Ð : lo=ð */
  { 0xc391 , 0xc391 , 0xc3b1 , None , "N" }, /* Ñ : up=Ñ : lo=ñ */
  { 0xc392 , 0xc392 , 0xc3b2 , None , "O" }, /* Ò : up=Ò : lo=ò */
  { 0xc393 , 0xc393 , 0xc3b3 , None , "O" }, /* Ó : up=Ó : lo=ó */
  { 0xc394 , 0xc394 , 0xc3b4 , None , "O" }, /* Ô : up=Ô : lo=ô */
  { 0xc395 , 0xc395 , 0xc3b5 , None , "O" }, /* Õ : up=Õ : lo=õ */
  { 0xc396 , 0xc396 , 0xc3b6 , None , "O" }, /* Ö : up=Ö : lo=ö */
  { 0xc397 , 0xc397 , 0xc397 , None , "x" }, /* × : up=× : lo=× */
  { 0xc398 , 0xc398 , 0xc3b8 , None , "O" }, /* Ø : up=Ø : lo=ø */
  { 0xc399 , 0xc399 , 0xc3b9 , None , "U" }, /* Ù : up=Ù : lo=ù */
  { 0xc39a , 0xc39a , 0xc3ba , None , "U" }, /* Ú : up=Ú : lo=ú */
  { 0xc39b , 0xc39b , 0xc3bb , None , "U" }, /* Û : up=Û : lo=û */
  { 0xc39c , 0xc39c , 0xc3bc , None , "U" }, /* Ü : up=Ü : lo=ü */
  { 0xc39d , 0xc39d , 0xc3bd , None , "Y" }, /* Ý : up=Ý : lo=ý */
  { 0xc39e , 0xc39e , 0xc3be , None , "TH" }, /* Þ : up=Þ : lo=þ */
  { 0xc39f , 0xc39f , 0xc39f , None , "ss" }, /* ß : up=ß : lo=ß */
  { 0xc3a0 , 0xc380 , 0xc3a0 , None , "a" }, /* à : up=À : lo=à */
  { 0xc3a1 , 0xc381 , 0xc3a1 , None , "a" }, /* á : up=Á : lo=á */
  { 0xc3a2 , 0xc382 , 0xc3a2 , None , "a" }, /* â : up=Â : lo=â */
  { 0xc3a3 , 0xc383 , 0xc3a3 , None , "a" }, /* ã : up=Ã : lo=ã */
  { 0xc3a4 , 0xc384 , 0xc3a4 , None , "a" }, /* ä : up=Ä : lo=ä */
  { 0xc3a5 , 0xc385 , 0xc3a5 , None , "a" }, /* å : up=Å : lo=å */
  { 0xc3a6 , 0xc386 , 0xc3a6 , None , "ae" }, /* æ : up=Æ : lo=æ */
  { 0xc3a7 , 0xc387 , 0xc3a7 , None , "c" }, /* ç : up=Ç : lo=ç */
  { 0xc3a8 , 0xc388 , 0xc3a8 , None , "e" }, /* è : up=È : lo=è */
  { 0xc3a9 , 0xc389 , 0xc3a9 , None , "e" }, /* é : up=É : lo=é */
  { 0xc3aa , 0xc38a , 0xc3aa , None , "e" }, /* ê : up=Ê : lo=ê */
  { 0xc3ab , 0xc38b , 0xc3ab , None , "e" }, /* ë : up=Ë : lo=ë */
  { 0xc3ac , 0xc38c , 0xc3ac , None , "i" }, /* ì : up=Ì : lo=ì */
  { 0xc3ad , 0xc38d , 0xc3ad , None , "i" }, /* í : up=Í : lo=í */
  { 0xc3ae , 0xc38e , 0xc3ae , None , "i" }, /* î : up=Î : lo=î */
  { 0xc3af , 0xc38f , 0xc3af , None , "i" }, /* ï : up=Ï : lo=ï */
  { 0xc3b0 , 0xc390 , 0xc3b0 , None , "d" }, /* ð : up=Ð : lo=ð */
  { 0xc3b1 , 0xc391 , 0xc3b1 , None , "n" }, /* ñ : up=Ñ : lo=ñ */
  { 0xc3b2 , 0xc392 , 0xc3b2 , None , "o" }, /* ò : up=Ò : lo=ò */
  { 0xc3b3 , 0xc393 , 0xc3b3 , None , "o" }, /* ó : up=Ó : lo=ó */
  { 0xc3b4 , 0xc394 , 0xc3b4 , None , "o" }, /* ô : up=Ô : lo=ô */
  { 0xc3b5 , 0xc395 , 0xc3b5 , None , "o" }, /* õ : up=Õ : lo=õ */
  { 0xc3b6 , 0xc396 , 0xc3b6 , None , "o" }, /* ö : up=Ö : lo=ö */
  { 0xc3b7 , 0xc3b7 , 0xc3b7 , None , u8"\xc3\xb7" }, /* ÷ : up=÷ : lo=÷ */
  { 0xc3b8 , 0xc398 , 0xc3b8 , None , "o" }, /* ø : up=Ø : lo=ø */
  { 0xc3b9 , 0xc399 , 0xc3b9 , None , "u" }, /* ù : up=Ù : lo=ù */
  { 0xc3ba , 0xc39a , 0xc3ba , None , "u" }, /* ú : up=Ú : lo=ú */
  { 0xc3bb , 0xc39b , 0xc3bb , None , "u" }, /* û : up=Û : lo=û */
  { 0xc3bc , 0xc39c , 0xc3bc , None , "u" }, /* ü : up=Ü : lo=ü */
  { 0xc3bd , 0xc39d , 0xc3bd , None , "y" }, /* ý : up=Ý : lo=ý */
  { 0xc3be , 0xc39e , 0xc3be , None , "th" }, /* þ : up=Þ : lo=þ */
  { 0xc3bf , 0xc5b8 , 0xc3bf , None , "y" }, /* ÿ : up=Ÿ : lo=ÿ */
};

const character charmap_c4[64] = {
  { 0xc480 , 0xc480 , 0xc481 , None , "A" }, /* Ā : up=Ā : lo=ā */
  { 0xc481 , 0xc480 , 0xc481 , None , "a" }, /* ā : up=Ā : lo=ā */
  { 0xc482 , 0xc482 , 0xc483 , None , "A" }, /* Ă : up=Ă : lo=ă */
  { 0xc483 , 0xc482 , 0xc483 , None , "a" }, /* ă : up=Ă : lo=ă */
  { 0xc484 , 0xc484 , 0xc485 , None , "A" }, /* Ą : up=Ą : lo=ą */
  { 0xc485 , 0xc484 , 0xc485 , None , "a" }, /* ą : up=Ą : lo=ą */
  { 0xc486 , 0xc486 , 0xc487 , None , "C" }, /* Ć : up=Ć : lo=ć */
  { 0xc487 , 0xc486 , 0xc487 , None , "c" }, /* ć : up=Ć : lo=ć */
  { 0xc488 , 0xc488 , 0xc489 , None , "C" }, /* Ĉ : up=Ĉ : lo=ĉ */
  { 0xc489 , 0xc488 , 0xc489 , None , "c" }, /* ĉ : up=Ĉ : lo=ĉ */
  { 0xc48a , 0xc48a , 0xc48b , None , "C" }, /* Ċ : up=Ċ : lo=ċ */
  { 0xc48b , 0xc48a , 0xc48b , None , "c" }, /* ċ : up=Ċ : lo=ċ */
  { 0xc48c , 0xc48c , 0xc48d , None , "C" }, /* Č : up=Č : lo=č */
  { 0xc48d , 0xc48c , 0xc48d , None , "c" }, /* č : up=Č : lo=č */
  { 0xc48e , 0xc48e , 0xc48f , None , "D" }, /* Ď : up=Ď : lo=ď */
  { 0xc48f , 0xc48e , 0xc48f , None , "d" }, /* ď : up=Ď : lo=ď */
  { 0xc490 , 0xc490 , 0xc491 , None , "D" }, /* Đ : up=Đ : lo=đ */
  { 0xc491 , 0xc490 , 0xc491 , None , "d" }, /* đ : up=Đ : lo=đ */
  { 0xc492 , 0xc492 , 0xc493 , None , "E" }, /* Ē : up=Ē : lo=ē */
  { 0xc493 , 0xc492 , 0xc493 , None , "e" }, /* ē : up=Ē : lo=ē */
  { 0xc494 , 0xc494 , 0xc495 , None , "E" }, /* Ĕ : up=Ĕ : lo=ĕ */
  { 0xc495 , 0xc494 , 0xc495 , None , "e" }, /* ĕ : up=Ĕ : lo=ĕ */
  { 0xc496 , 0xc496 , 0xc497 , None , "E" }, /* Ė : up=Ė : lo=ė */
  { 0xc497 , 0xc496 , 0xc497 , None , "e" }, /* ė : up=Ė : lo=ė */
  { 0xc498 , 0xc498 , 0xc499 , None , "E" }, /* Ę : up=Ę : lo=ę */
  { 0xc499 , 0xc498 , 0xc499 , None , "e" }, /* ę : up=Ę : lo=ę */
  { 0xc49a , 0xc49a , 0xc49b , None , "E" }, /* Ě : up=Ě : lo=ě */
  { 0xc49b , 0xc49a , 0xc49b , None , "e" }, /* ě : up=Ě : lo=ě */
  { 0xc49c , 0xc49c , 0xc49d , None , "G" }, /* Ĝ : up=Ĝ : lo=ĝ */
  { 0xc49d , 0xc49c , 0xc49d , None , "g" }, /* ĝ : up=Ĝ : lo=ĝ */
  { 0xc49e , 0xc49e , 0xc49f , None , "G" }, /* Ğ : up=Ğ : lo=ğ */
  { 0xc49f , 0xc49e , 0xc49f , None , "g" }, /* ğ : up=Ğ : lo=ğ */
  { 0xc4a0 , 0xc4a0 , 0xc4a1 , None , "G" }, /* Ġ : up=Ġ : lo=ġ */
  { 0xc4a1 , 0xc4a0 , 0xc4a1 , None , "g" }, /* ġ : up=Ġ : lo=ġ */
  { 0xc4a2 , 0xc4a2 , 0xc4a3 , None , "G" }, /* Ģ : up=Ģ : lo=ģ */
  { 0xc4a3 , 0xc4a2 , 0xc4a3 , None , "g" }, /* ģ : up=Ģ : lo=ģ */
  { 0xc4a4 , 0xc4a4 , 0xc4a5 , None , "H" }, /* Ĥ : up=Ĥ : lo=ĥ */
  { 0xc4a5 , 0xc4a4 , 0xc4a5 , None , "h" }, /* ĥ : up=Ĥ : lo=ĥ */
  { 0xc4a6 , 0xc4a6 , 0xc4a7 , None , "H" }, /* Ħ : up=Ħ : lo=ħ */
  { 0xc4a7 , 0xc4a6 , 0xc4a7 , None , "h" }, /* ħ : up=Ħ : lo=ħ */
  { 0xc4a8 , 0xc4a8 , 0xc4a9 , None , "I" }, /* Ĩ : up=Ĩ : lo=ĩ */
  { 0xc4a9 , 0xc4a8 , 0xc4a9 , None , "i" }, /* ĩ : up=Ĩ : lo=ĩ */
  { 0xc4aa , 0xc4aa , 0xc4ab , None , "I" }, /* Ī : up=Ī : lo=ī */
  { 0xc4ab , 0xc4aa , 0xc4ab , None , "i" }, /* ī : up=Ī : lo=ī */
  { 0xc4ac , 0xc4ac , 0xc4ad , None , "I" }, /* Ĭ : up=Ĭ : lo=ĭ */
  { 0xc4ad , 0xc4ac , 0xc4ad , None , "i" }, /* ĭ : up=Ĭ : lo=ĭ */
  { 0xc4ae , 0xc4ae , 0xc4af , None , "I" }, /* Į : up=Į : lo=į */
  { 0xc4af , 0xc4ae , 0xc4af , None , "i" }, /* į : up=Į : lo=į */
  { 0xc4b0 , 0xc4b0 , 0x69 , None , "I" }, /* İ : up=İ : lo=i */
  { 0xc4b1 , 0x49 , 0xc4b1 , None , "i" }, /* ı : up=I : lo=ı */
  { 0xc4b2 , 0xc4b2 , 0xc4b3 , None , "IJ" }, /* Ĳ : up=Ĳ : lo=ĳ */
  { 0xc4b3 , 0xc4b2 , 0xc4b3 , None , "ij" }, /* ĳ : up=Ĳ : lo=ĳ */
  { 0xc4b4 , 0xc4b4 , 0xc4b5 , None , "J" }, /* Ĵ : up=Ĵ : lo=ĵ */
  { 0xc4b5 , 0xc4b4 , 0xc4b5 , None , "j" }, /* ĵ : up=Ĵ : lo=ĵ */
  { 0xc4b6 , 0xc4b6 , 0xc4b7 , None , "K" }, /* Ķ : up=Ķ : lo=ķ */
  { 0xc4b7 , 0xc4b6 , 0xc4b7 , None , "k" }, /* ķ : up=Ķ : lo=ķ */
  { 0xc4b8 , 0xc4b8 , 0xc4b8 , None , "q" }, /* ĸ : up=ĸ : lo=ĸ */
  { 0xc4b9 , 0xc4b9 , 0xc4ba , None , "L" }, /* Ĺ : up=Ĺ : lo=ĺ */
  { 0xc4ba , 0xc4b9 , 0xc4ba , None , "l" }, /* ĺ : up=Ĺ : lo=ĺ */
  { 0xc4bb , 0xc4bb , 0xc4bc , None , "L" }, /* Ļ : up=Ļ : lo=ļ */
  { 0xc4bc , 0xc4bb , 0xc4bc , None , "l" }, /* ļ : up=Ļ : lo=ļ */
  { 0xc4bd , 0xc4bd , 0xc4be , None , "L" }, /* Ľ : up=Ľ : lo=ľ */
  { 0xc4be , 0xc4bd , 0xc4be , None , "l" }, /* ľ : up=Ľ : lo=ľ */
  { 0xc4bf , 0xc4bf , 0xc580 , None , "L" }, /* Ŀ : up=Ŀ : lo=ŀ */
};

const character charmap_c5[64] = {
  { 0xc580 , 0xc4bf , 0xc580 , None , "l" }, /* ŀ : up=Ŀ : lo=ŀ */
  { 0xc581 , 0xc581 , 0xc582 , None , "L" }, /* Ł : up=Ł : lo=ł */
  { 0xc582 , 0xc581 , 0xc582 , None , "l" }, /* ł : up=Ł : lo=ł */
  { 0xc583 , 0xc583 , 0xc584 , None , "N" }, /* Ń : up=Ń : lo=ń */
  { 0xc584 , 0xc583 , 0xc584 , None , "n" }, /* ń : up=Ń : lo=ń */
  { 0xc585 , 0xc585 , 0xc586 , None , "N" }, /* Ņ : up=Ņ : lo=ņ */
  { 0xc586 , 0xc585 , 0xc586 , None , "n" }, /* ņ : up=Ņ : lo=ņ */
  { 0xc587 , 0xc587 , 0xc588 , None , "N" }, /* Ň : up=Ň : lo=ň */
  { 0xc588 , 0xc587 , 0xc588 , None , "n" }, /* ň : up=Ň : lo=ň */
  { 0xc589 , 0xc589 , 0xc589 , None , u8"\x27\x6e" }, /* ŉ : up=ŉ : lo=ŉ */
  { 0xc58a , 0xc58a , 0xc58b , None , "N" }, /* Ŋ : up=Ŋ : lo=ŋ */
  { 0xc58b , 0xc58a , 0xc58b , None , "n" }, /* ŋ : up=Ŋ : lo=ŋ */
  { 0xc58c , 0xc58c , 0xc58d , None , "O" }, /* Ō : up=Ō : lo=ō */
  { 0xc58d , 0xc58c , 0xc58d , None , "o" }, /* ō : up=Ō : lo=ō */
  { 0xc58e , 0xc58e , 0xc58f , None , "O" }, /* Ŏ : up=Ŏ : lo=ŏ */
  { 0xc58f , 0xc58e , 0xc58f , None , "o" }, /* ŏ : up=Ŏ : lo=ŏ */
  { 0xc590 , 0xc590 , 0xc591 , None , "O" }, /* Ő : up=Ő : lo=ő */
  { 0xc591 , 0xc590 , 0xc591 , None , "o" }, /* ő : up=Ő : lo=ő */
  { 0xc592 , 0xc592 , 0xc593 , None , "OE" }, /* Œ : up=Œ : lo=œ */
  { 0xc593 , 0xc592 , 0xc593 , None , "oe" }, /* œ : up=Œ : lo=œ */
  { 0xc594 , 0xc594 , 0xc595 , None , "R" }, /* Ŕ : up=Ŕ : lo=ŕ */
  { 0xc595 , 0xc594 , 0xc595 , None , "r" }, /* ŕ : up=Ŕ : lo=ŕ */
  { 0xc596 , 0xc596 , 0xc597 , None , "R" }, /* Ŗ : up=Ŗ : lo=ŗ */
  { 0xc597 , 0xc596 , 0xc597 , None , "r" }, /* ŗ : up=Ŗ : lo=ŗ */
  { 0xc598 , 0xc598 , 0xc599 , None , "R" }, /* Ř : up=Ř : lo=ř */
  { 0xc599 , 0xc598 , 0xc599 , None , "r" }, /* ř : up=Ř : lo=ř */
  { 0xc59a , 0xc59a , 0xc59b , None , "S" }, /* Ś : up=Ś : lo=ś */
  { 0xc59b , 0xc59a , 0xc59b , None , "s" }, /* ś : up=Ś : lo=ś */
  { 0xc59c , 0xc59c , 0xc59d , None , "S" }, /* Ŝ : up=Ŝ : lo=ŝ */
  { 0xc59d , 0xc59c , 0xc59d , None , "s" }, /* ŝ : up=Ŝ : lo=ŝ */
  { 0xc59e , 0xc59e , 0xc59f , None , "S" }, /* Ş : up=Ş : lo=ş */
  { 0xc59f , 0xc59e , 0xc59f , None , "s" }, /* ş : up=Ş : lo=ş */
  { 0xc5a0 , 0xc5a0 , 0xc5a1 , None , "S" }, /* Š : up=Š : lo=š */
  { 0xc5a1 , 0xc5a0 , 0xc5a1 , None , "s" }, /* š : up=Š : lo=š */
  { 0xc5a2 , 0xc5a2 , 0xc5a3 , None , "T" }, /* Ţ : up=Ţ : lo=ţ */
  { 0xc5a3 , 0xc5a2 , 0xc5a3 , None , "t" }, /* ţ : up=Ţ : lo=ţ */
  { 0xc5a4 , 0xc5a4 , 0xc5a5 , None , "T" }, /* Ť : up=Ť : lo=ť */
  { 0xc5a5 , 0xc5a4 , 0xc5a5 , None , "t" }, /* ť : up=Ť : lo=ť */
  { 0xc5a6 , 0xc5a6 , 0xc5a7 , None , "T" }, /* Ŧ : up=Ŧ : lo=ŧ */
  { 0xc5a7 , 0xc5a6 , 0xc5a7 , None , "t" }, /* ŧ : up=Ŧ : lo=ŧ */
  { 0xc5a8 , 0xc5a8 , 0xc5a9 , None , "U" }, /* Ũ : up=Ũ : lo=ũ */
  { 0xc5a9 , 0xc5a8 , 0xc5a9 , None , "u" }, /* ũ : up=Ũ : lo=ũ */
  { 0xc5aa , 0xc5aa , 0xc5ab , None , "U" }, /* Ū : up=Ū : lo=ū */
  { 0xc5ab , 0xc5aa , 0xc5ab , None , "u" }, /* ū : up=Ū : lo=ū */
  { 0xc5ac , 0xc5ac , 0xc5ad , None , "U" }, /* Ŭ : up=Ŭ : lo=ŭ */
  { 0xc5ad , 0xc5ac , 0xc5ad , None , "u" }, /* ŭ : up=Ŭ : lo=ŭ */
  { 0xc5ae , 0xc5ae , 0xc5af , None , "U" }, /* Ů : up=Ů : lo=ů */
  { 0xc5af , 0xc5ae , 0xc5af , None , "u" }, /* ů : up=Ů : lo=ů */
  { 0xc5b0 , 0xc5b0 , 0xc5b1 , None , "U" }, /* Ű : up=Ű : lo=ű */
  { 0xc5b1 , 0xc5b0 , 0xc5b1 , None , "u" }, /* ű : up=Ű : lo=ű */
  { 0xc5b2 , 0xc5b2 , 0xc5b3 , None , "U" }, /* Ų : up=Ų : lo=ų */
  { 0xc5b3 , 0xc5b2 , 0xc5b3 , None , "u" }, /* ų : up=Ų : lo=ų */
  { 0xc5b4 , 0xc5b4 , 0xc5b5 , None , "W" }, /* Ŵ : up=Ŵ : lo=ŵ */
  { 0xc5b5 , 0xc5b4 , 0xc5b5 , None , "w" }, /* ŵ : up=Ŵ : lo=ŵ */
  { 0xc5b6 , 0xc5b6 , 0xc5b7 , None , "Y" }, /* Ŷ : up=Ŷ : lo=ŷ */
  { 0xc5b7 , 0xc5b6 , 0xc5b7 , None , "y" }, /* ŷ : up=Ŷ : lo=ŷ */
  { 0xc5b8 , 0xc5b8 , 0xc3bf , None , "Y" }, /* Ÿ : up=Ÿ : lo=ÿ */
  { 0xc5b9 , 0xc5b9 , 0xc5ba , None , "Z" }, /* Ź : up=Ź : lo=ź */
  { 0xc5ba , 0xc5b9 , 0xc5ba , None , "z" }, /* ź : up=Ź : lo=ź */
  { 0xc5bb , 0xc5bb , 0xc5bc , None , "Z" }, /* Ż : up=Ż : lo=ż */
  { 0xc5bc , 0xc5bb , 0xc5bc , None , "z" }, /* ż : up=Ż : lo=ż */
  { 0xc5bd , 0xc5bd , 0xc5be , None , "Z" }, /* Ž : up=Ž : lo=ž */
  { 0xc5be , 0xc5bd , 0xc5be , None , "z" }, /* ž : up=Ž : lo=ž */
  { 0xc5bf , 0x53 , 0xc5bf , None , "s" }, /* ſ : up=S : lo=ſ */
};

const character charmap_c6[64] = {
  { 0xc680 , 0xc983 , 0xc680 , None , "b" }, /* ƀ : up=Ƀ : lo=ƀ */
  { 0xc681 , 0xc681 , 0xc993 , None , "B" }, /* Ɓ : up=Ɓ : lo=ɓ */
  { 0xc682 , 0xc682 , 0xc683 , None , "B" }, /* Ƃ : up=Ƃ : lo=ƃ */
  { 0xc683 , 0xc682 , 0xc683 , None , "b" }, /* ƃ : up=Ƃ : lo=ƃ */
  { 0xc684 , 0xc684 , 0xc685 , None , "b" }, /* Ƅ : up=Ƅ : lo=ƅ */
  { 0xc685 , 0xc684 , 0xc685 , None , "B" }, /* ƅ : up=Ƅ : lo=ƅ */
  { 0xc686 , 0xc686 , 0xc994 , None , u8"\xc6\x86" }, /* Ɔ : up=Ɔ : lo=ɔ */
  { 0xc687 , 0xc687 , 0xc688 , None , "C" }, /* Ƈ : up=Ƈ : lo=ƈ */
  { 0xc688 , 0xc687 , 0xc688 , None , "c" }, /* ƈ : up=Ƈ : lo=ƈ */
  { 0xc689 , 0xc689 , 0xc996 , None , "D" }, /* Ɖ : up=Ɖ : lo=ɖ */
  { 0xc68a , 0xc68a , 0xc997 , None , "D" }, /* Ɗ : up=Ɗ : lo=ɗ */
  { 0xc68b , 0xc68b , 0xc68c , None , "D" }, /* Ƌ : up=Ƌ : lo=ƌ */
  { 0xc68c , 0xc68b , 0xc68c , None , "d" }, /* ƌ : up=Ƌ : lo=ƌ */
  { 0xc68d , 0xc68d , 0xc68d , None , u8"\xc6\x8d" }, /* ƍ : up=ƍ : lo=ƍ */
  { 0xc68e , 0xc68e , 0xc79d , None , u8"\xc6\x8e" }, /* Ǝ : up=Ǝ : lo=ǝ */
  { 0xc68f , 0xc68f , 0xc999 , None , u8"\xc6\x8f" }, /* Ə : up=Ə : lo=ə */
  { 0xc690 , 0xc690 , 0xc99b , None , "E" }, /* Ɛ : up=Ɛ : lo=ɛ */
  { 0xc691 , 0xc691 , 0xc692 , None , "F" }, /* Ƒ : up=Ƒ : lo=ƒ */
  { 0xc692 , 0xc691 , 0xc692 , None , "f" }, /* ƒ : up=Ƒ : lo=ƒ */
  { 0xc693 , 0xc693 , 0xc9a0 , None , "G" }, /* Ɠ : up=Ɠ : lo=ɠ */
  { 0xc694 , 0xc694 , 0xc9a3 , None , u8"\xc6\x94" }, /* Ɣ : up=Ɣ : lo=ɣ */
  { 0xc695 , 0xc7b6 , 0xc695 , None , "hv" }, /* ƕ : up=Ƕ : lo=ƕ */
  { 0xc696 , 0xc696 , 0xc9a9 , None , "I" }, /* Ɩ : up=Ɩ : lo=ɩ */
  { 0xc697 , 0xc697 , 0xc9a8 , None , "I" }, /* Ɨ : up=Ɨ : lo=ɨ */
  { 0xc698 , 0xc698 , 0xc699 , None , "K" }, /* Ƙ : up=Ƙ : lo=ƙ */
  { 0xc699 , 0xc698 , 0xc699 , None , "k" }, /* ƙ : up=Ƙ : lo=ƙ */
  { 0xc69a , 0xc8bd , 0xc69a , None , "l" }, /* ƚ : up=Ƚ : lo=ƚ */
  { 0xc69b , 0xc69b , 0xc69b , None , u8"\xc6\x9b" }, /* ƛ : up=ƛ : lo=ƛ */
  { 0xc69c , 0xc69c , 0xc9af , None , u8"\xc6\x9c" }, /* Ɯ : up=Ɯ : lo=ɯ */
  { 0xc69d , 0xc69d , 0xc9b2 , None , "N" }, /* Ɲ : up=Ɲ : lo=ɲ */
  { 0xc69e , 0xc8a0 , 0xc69e , None , "n" }, /* ƞ : up=Ƞ : lo=ƞ */
  { 0xc69f , 0xc69f , 0xc9b5 , None , u8"\xc6\x9f" }, /* Ɵ : up=Ɵ : lo=ɵ */
  { 0xc6a0 , 0xc6a0 , 0xc6a1 , None , u8"\xc6\xa0" }, /* Ơ : up=Ơ : lo=ơ */
  { 0xc6a1 , 0xc6a0 , 0xc6a1 , None , u8"\xc6\xa1" }, /* ơ : up=Ơ : lo=ơ */
  { 0xc6a2 , 0xc6a2 , 0xc6a3 , None , "OI" }, /* Ƣ : up=Ƣ : lo=ƣ */
  { 0xc6a3 , 0xc6a2 , 0xc6a3 , None , "oi" }, /* ƣ : up=Ƣ : lo=ƣ */
  { 0xc6a4 , 0xc6a4 , 0xc6a5 , None , "P" }, /* Ƥ : up=Ƥ : lo=ƥ */
  { 0xc6a5 , 0xc6a4 , 0xc6a5 , None , "p" }, /* ƥ : up=Ƥ : lo=ƥ */
  { 0xc6a6 , 0xc6a6 , 0xca80 , None , "R" }, /* Ʀ : up=Ʀ : lo=ʀ */
  { 0xc6a7 , 0xc6a7 , 0xc6a8 , None , u8"\xc6\xa7" }, /* Ƨ : up=Ƨ : lo=ƨ */
  { 0xc6a8 , 0xc6a7 , 0xc6a8 , None , u8"\xc6\xa8" }, /* ƨ : up=Ƨ : lo=ƨ */
  { 0xc6a9 , 0xc6a9 , 0xca83 , None , u8"\xc6\xa9" }, /* Ʃ : up=Ʃ : lo=ʃ */
  { 0xc6aa , 0xc6aa , 0xc6aa , None , u8"\xc6\xaa" }, /* ƪ : up=ƪ : lo=ƪ */
  { 0xc6ab , 0xc6ab , 0xc6ab , None , "t" }, /* ƫ : up=ƫ : lo=ƫ */
  { 0xc6ac , 0xc6ac , 0xc6ad , None , "T" }, /* Ƭ : up=Ƭ : lo=ƭ */
  { 0xc6ad , 0xc6ac , 0xc6ad , None , "t" }, /* ƭ : up=Ƭ : lo=ƭ */
  { 0xc6ae , 0xc6ae , 0xca88 , None , "T" }, /* Ʈ : up=Ʈ : lo=ʈ */
  { 0xc6af , 0xc6af , 0xc6b0 , None , "U" }, /* Ư : up=Ư : lo=ư */
  { 0xc6b0 , 0xc6af , 0xc6b0 , None , "u" }, /* ư : up=Ư : lo=ư */
  { 0xc6b1 , 0xc6b1 , 0xca8a , None , u8"\xc6\xb1" }, /* Ʊ : up=Ʊ : lo=ʊ */
  { 0xc6b2 , 0xc6b2 , 0xca8b , None , "V" }, /* Ʋ : up=Ʋ : lo=ʋ */
  { 0xc6b3 , 0xc6b3 , 0xc6b4 , None , "Y" }, /* Ƴ : up=Ƴ : lo=ƴ */
  { 0xc6b4 , 0xc6b3 , 0xc6b4 , None , "y" }, /* ƴ : up=Ƴ : lo=ƴ */
  { 0xc6b5 , 0xc6b5 , 0xc6b6 , None , "Z" }, /* Ƶ : up=Ƶ : lo=ƶ */
  { 0xc6b6 , 0xc6b5 , 0xc6b6 , None , "z" }, /* ƶ : up=Ƶ : lo=ƶ */
  { 0xc6b7 , 0xc6b7 , 0xca92 , None , u8"\xc6\xb7" }, /* Ʒ : up=Ʒ : lo=ʒ */
  { 0xc6b8 , 0xc6b8 , 0xc6b9 , None , u8"\xc6\xb8" }, /* Ƹ : up=Ƹ : lo=ƹ */
  { 0xc6b9 , 0xc6b8 , 0xc6b9 , None , u8"\xc6\xb9" }, /* ƹ : up=Ƹ : lo=ƹ */
  { 0xc6ba , 0xc6ba , 0xc6ba , None , u8"\xc6\xba" }, /* ƺ : up=ƺ : lo=ƺ */
  { 0xc6bb , 0xc6bb , 0xc6bb , None , u8"\xc6\xbb" }, /* ƻ : up=ƻ : lo=ƻ */
  { 0xc6bc , 0xc6bc , 0xc6bd , None , u8"\xc6\xbc" }, /* Ƽ : up=Ƽ : lo=ƽ */
  { 0xc6bd , 0xc6bc , 0xc6bd , None , u8"\xc6\xbd" }, /* ƽ : up=Ƽ : lo=ƽ */
  { 0xc6be , 0xc6be , 0xc6be , None , u8"\xc6\xbe" }, /* ƾ : up=ƾ : lo=ƾ */
  { 0xc6bf , 0xc7b7 , 0xc6bf , None , u8"\xc6\xbf" }, /* ƿ : up=Ƿ : lo=ƿ */
};

const character charmap_c7[64] = {
  { 0xc780 , 0xc780 , 0xc780 , None , u8"\xc7\x80" }, /* ǀ : up=ǀ : lo=ǀ */
  { 0xc781 , 0xc781 , 0xc781 , None , u8"\xc7\x81" }, /* ǁ : up=ǁ : lo=ǁ */
  { 0xc782 , 0xc782 , 0xc782 , None , u8"\xc7\x82" }, /* ǂ : up=ǂ : lo=ǂ */
  { 0xc783 , 0xc783 , 0xc783 , None , u8"\xc7\x83" }, /* ǃ : up=ǃ : lo=ǃ */
  { 0xc784 , 0xc784 , 0xc786 , None , u8"\xc7\x84" }, /* Ǆ : up=Ǆ : lo=ǆ */
  { 0xc785 , 0xc784 , 0xc786 , None , u8"\xc7\x85" }, /* ǅ : up=Ǆ : lo=ǆ */
  { 0xc786 , 0xc784 , 0xc786 , None , u8"\xc7\x86" }, /* ǆ : up=Ǆ : lo=ǆ */
  { 0xc787 , 0xc787 , 0xc789 , None , "LJ" }, /* Ǉ : up=Ǉ : lo=ǉ */
  { 0xc788 , 0xc787 , 0xc789 , None , "Lj" }, /* ǈ : up=Ǉ : lo=ǉ */
  { 0xc789 , 0xc787 , 0xc789 , None , "lj" }, /* ǉ : up=Ǉ : lo=ǉ */
  { 0xc78a , 0xc78a , 0xc78c , None , "NJ" }, /* ǋ : up=Ǌ : lo=ǌ */
  { 0xc78b , 0xc78a , 0xc78c , None , "Nj" }, /* ǋ : up=Ǌ : lo=ǌ */
  { 0xc78c , 0xc78a , 0xc78c , None , "nj" }, /* ǌ : up=Ǌ : lo=ǌ */
  { 0xc78d , 0xc78d , 0xc78e , None , "A" }, /* Ǎ : up=Ǎ : lo=ǎ */
  { 0xc78e , 0xc78d , 0xc78e , None , "a" }, /* ǎ : up=Ǎ : lo=ǎ */
  { 0xc78f , 0xc78f , 0xc790 , None , "I" }, /* Ǐ : up=Ǐ : lo=ǐ */
  { 0xc790 , 0xc78f , 0xc790 , None , "i" }, /* ǐ : up=Ǐ : lo=ǐ */
  { 0xc791 , 0xc791 , 0xc792 , None , "O" }, /* Ǒ : up=Ǒ : lo=ǒ */
  { 0xc792 , 0xc791 , 0xc792 , None , "o" }, /* ǒ : up=Ǒ : lo=ǒ */
  { 0xc793 , 0xc793 , 0xc794 , None , "U" }, /* Ǔ : up=Ǔ : lo=ǔ */
  { 0xc794 , 0xc793 , 0xc794 , None , "u" }, /* ǔ : up=Ǔ : lo=ǔ */
  { 0xc795 , 0xc795 , 0xc796 , None , "U" }, /* Ǖ : up=Ǖ : lo=ǖ */
  { 0xc796 , 0xc795 , 0xc796 , None , "u" }, /* ǖ : up=Ǖ : lo=ǖ */
  { 0xc797 , 0xc797 , 0xc798 , None , "U" }, /* Ǘ : up=Ǘ : lo=ǘ */
  { 0xc798 , 0xc797 , 0xc798 , None , "U" }, /* Ǘ : up=Ǘ : lo=ǘ */
  { 0xc799 , 0xc799 , 0xc79a , None , "U" }, /* Ǚ : up=Ǚ : lo=ǚ */
  { 0xc79a , 0xc799 , 0xc79a , None , "u" }, /* ǚ : up=Ǚ : lo=ǚ */
  { 0xc79b , 0xc79b , 0xc79c , None , "U" }, /* Ǜ : up=Ǜ : lo=ǜ */
  { 0xc79c , 0xc79b , 0xc79c , None , "u" }, /* ǜ : up=Ǜ : lo=ǜ */
  { 0xc79d , 0xc68e , 0xc79d , None , u8"\xc7\x9d" }, /* ǝ : up=Ǝ : lo=ǝ */
  { 0xc79e , 0xc79e , 0xc79f , None , "A" }, /* Ǟ : up=Ǟ : lo=ǟ */
  { 0xc79f , 0xc79e , 0xc79f , None , "a" }, /* ǟ : up=Ǟ : lo=ǟ */
  { 0xc7a0 , 0xc7a0 , 0xc7a1 , None , "A" }, /* Ǡ : up=Ǡ : lo=ǡ */
  { 0xc7a1 , 0xc7a0 , 0xc7a1 , None , "A" }, /* Ǡ : up=Ǡ : lo=ǡ */
  { 0xc7a2 , 0xc7a2 , 0xc7a3 , None , "AE" }, /* Ǣ : up=Ǣ : lo=ǣ */
  { 0xc7a3 , 0xc7a2 , 0xc7a3 , None , "ae" }, /* ǣ : up=Ǣ : lo=ǣ */
  { 0xc7a4 , 0xc7a4 , 0xc7a5 , None , "G" }, /* Ǥ : up=Ǥ : lo=ǥ */
  { 0xc7a5 , 0xc7a4 , 0xc7a5 , None , "g" }, /* ǥ : up=Ǥ : lo=ǥ */
  { 0xc7a6 , 0xc7a6 , 0xc7a7 , None , "G" }, /* Ǧ : up=Ǧ : lo=ǧ */
  { 0xc7a7 , 0xc7a6 , 0xc7a7 , None , "g" }, /* ǧ : up=Ǧ : lo=ǧ */
  { 0xc7a8 , 0xc7a8 , 0xc7a9 , None , "K" }, /* Ǩ : up=Ǩ : lo=ǩ */
  { 0xc7a9 , 0xc7a8 , 0xc7a9 , None , "k" }, /* ǩ : up=Ǩ : lo=ǩ */
  { 0xc7aa , 0xc7aa , 0xc7ab , None , "o" }, /* ǫ : up=Ǫ : lo=ǫ */
  { 0xc7ab , 0xc7aa , 0xc7ab , None , "o" }, /* ǫ : up=Ǫ : lo=ǫ */
  { 0xc7ac , 0xc7ac , 0xc7ad , None , "O" }, /* Ǭ : up=Ǭ : lo=ǭ */
  { 0xc7ad , 0xc7ac , 0xc7ad , None , "o" }, /* ǭ : up=Ǭ : lo=ǭ */
  { 0xc7ae , 0xc7ae , 0xc7af , None , u8"\xc7\xae" }, /* Ǯ : up=Ǯ : lo=ǯ */
  { 0xc7af , 0xc7ae , 0xc7af , None , u8"\xc7\xaf" }, /* ǯ : up=Ǯ : lo=ǯ */
  { 0xc7b0 , 0xc7b0 , 0xc7b0 , None , "J" }, /* ǰ : up=ǰ : lo=ǰ */
  { 0xc7b1 , 0xc7b1 , 0xc7b3 , None , "DZ" }, /* Ǳ : up=Ǳ : lo=ǳ */
  { 0xc7b2 , 0xc7b1 , 0xc7b3 , None , "Dz" }, /* ǲ : up=Ǳ : lo=ǳ */
  { 0xc7b3 , 0xc7b1 , 0xc7b3 , None , "dz" }, /* ǳ : up=Ǳ : lo=ǳ */
  { 0xc7b4 , 0xc7b4 , 0xc7b5 , None , "G" }, /* Ǵ : up=Ǵ : lo=ǵ */
  { 0xc7b5 , 0xc7b4 , 0xc7b5 , None , "g" }, /* ǵ : up=Ǵ : lo=ǵ */
  { 0xc7b6 , 0xc7b6 , 0xc695 , None , "HV" }, /* Ƕ : up=Ƕ : lo=ƕ */
  { 0xc7b7 , 0xc7b7 , 0xc6bf , None , u8"\xc7\xb7" }, /* Ƿ : up=Ƿ : lo=ƿ */
  { 0xc7b8 , 0xc7b8 , 0xc7b9 , None , "N" }, /* Ǹ : up=Ǹ : lo=ǹ */
  { 0xc7b9 , 0xc7b8 , 0xc7b9 , None , "n" }, /* ǹ : up=Ǹ : lo=ǹ */
  { 0xc7ba , 0xc7ba , 0xc7bb , None , "A" }, /* Ǻ : up=Ǻ : lo=ǻ */
  { 0xc7bb , 0xc7ba , 0xc7bb , None , "a" }, /* ǻ : up=Ǻ : lo=ǻ */
  { 0xc7bc , 0xc7bc , 0xc7bd , None , "AE" }, /* Ǽ : up=Ǽ : lo=ǽ */
  { 0xc7bd , 0xc7bc , 0xc7bd , None , "ae" }, /* ǽ : up=Ǽ : lo=ǽ */
  { 0xc7be , 0xc7be , 0xc7bf , None , "O" }, /* Ǿ : up=Ǿ : lo=ǿ */
  { 0xc7bf , 0xc7be , 0xc7bf , None , "o" }, /* ǿ : up=Ǿ : lo=ǿ */
};

const character charmap_c8[64] = {
  { 0xc880 , 0xc880 , 0xc881 , None , "A" }, /* Ȁ : up=Ȁ : lo=ȁ */
  { 0xc881 , 0xc880 , 0xc881 , None , "a" }, /* ȁ : up=Ȁ : lo=ȁ */
  { 0xc882 , 0xc882 , 0xc883 , None , "A" }, /* Ȃ : up=Ȃ : lo=ȃ */
  { 0xc883 , 0xc882 , 0xc883 , None , "a" }, /* ȃ : up=Ȃ : lo=ȃ */
  { 0xc884 , 0xc884 , 0xc885 , None , "E" }, /* Ȅ : up=Ȅ : lo=ȅ */
  { 0xc885 , 0xc884 , 0xc885 , None , "e" }, /* ȅ : up=Ȅ : lo=ȅ */
  { 0xc886 , 0xc886 , 0xc887 , None , "E" }, /* Ȇ : up=Ȇ : lo=ȇ */
  { 0xc887 , 0xc886 , 0xc887 , None , "e" }, /* ȇ : up=Ȇ : lo=ȇ */
  { 0xc888 , 0xc888 , 0xc889 , None , "I" }, /* Ȉ : up=Ȉ : lo=ȉ */
  { 0xc889 , 0xc888 , 0xc889 , None , "i" }, /* ȉ : up=Ȉ : lo=ȉ */
  { 0xc88a , 0xc88a , 0xc88b , None , "I" }, /* Ȋ : up=Ȋ : lo=ȋ */
  { 0xc88b , 0xc88a , 0xc88b , None , "i" }, /* ȋ : up=Ȋ : lo=ȋ */
  { 0xc88c , 0xc88c , 0xc88d , None , "O" }, /* Ȍ : up=Ȍ : lo=ȍ */
  { 0xc88d , 0xc88c , 0xc88d , None , "o" }, /* ȍ : up=Ȍ : lo=ȍ */
  { 0xc88e , 0xc88e , 0xc88f , None , "O" }, /* Ȏ : up=Ȏ : lo=ȏ */
  { 0xc88f , 0xc88e , 0xc88f , None , "o" }, /* ȏ : up=Ȏ : lo=ȏ */
  { 0xc890 , 0xc890 , 0xc891 , None , "R" }, /* Ȑ : up=Ȑ : lo=ȑ */
  { 0xc891 , 0xc890 , 0xc891 , None , "r" }, /* ȑ : up=Ȑ : lo=ȑ */
  { 0xc892 , 0xc892 , 0xc893 , None , "R" }, /* Ȓ : up=Ȓ : lo=ȓ */
  { 0xc893 , 0xc892 , 0xc893 , None , "r" }, /* ȓ : up=Ȓ : lo=ȓ */
  { 0xc894 , 0xc894 , 0xc895 , None , "U" }, /* Ȕ : up=Ȕ : lo=ȕ */
  { 0xc895 , 0xc894 , 0xc895 , None , "u" }, /* ȕ : up=Ȕ : lo=ȕ */
  { 0xc896 , 0xc896 , 0xc897 , None , "U" }, /* Ȗ : up=Ȗ : lo=ȗ */
  { 0xc897 , 0xc896 , 0xc897 , None , "u" }, /* ȗ : up=Ȗ : lo=ȗ */
  { 0xc898 , 0xc898 , 0xc899 , None , "S" }, /* Ș : up=Ș : lo=ș */
  { 0xc899 , 0xc898 , 0xc899 , None , "s" }, /* ș : up=Ș : lo=ș */
  { 0xc89a , 0xc89a , 0xc89b , None , "T" }, /* Ț : up=Ț : lo=ț */
  { 0xc89b , 0xc89a , 0xc89b , None , "t" }, /* ț : up=Ț : lo=ț */
  { 0xc89c , 0xc89c , 0xc89d , None , u8"\xc8\x9c" }, /* Ȝ : up=Ȝ : lo=ȝ */
  { 0xc89d , 0xc89c , 0xc89d , None , u8"\xc8\x9d" }, /* ȝ : up=Ȝ : lo=ȝ */
  { 0xc89e , 0xc89e , 0xc89f , None , "H" }, /* Ȟ : up=Ȟ : lo=ȟ */
  { 0xc89f , 0xc89e , 0xc89f , None , "h" }, /* ȟ : up=Ȟ : lo=ȟ */
  { 0xc8a0 , 0xc8a0 , 0xc69e , None , u8"\xc8\xa0" }, /* Ƞ : up=Ƞ : lo=ƞ */
  { 0xc8a1 , 0xc8a1 , 0xc8a1 , None , "d" }, /* ȡ : up=ȡ : lo=ȡ */
  { 0xc8a2 , 0xc8a2 , 0xc8a3 , None , u8"\xc8\xa2" }, /* Ȣ : up=Ȣ : lo=ȣ */
  { 0xc8a3 , 0xc8a2 , 0xc8a3 , None , u8"\xc8\xa3" }, /* ȣ : up=Ȣ : lo=ȣ */
  { 0xc8a4 , 0xc8a4 , 0xc8a5 , None , "Z" }, /* Ȥ : up=Ȥ : lo=ȥ */
  { 0xc8a5 , 0xc8a4 , 0xc8a5 , None , "z" }, /* ȥ : up=Ȥ : lo=ȥ */
  { 0xc8a6 , 0xc8a6 , 0xc8a7 , None , "A" }, /* Ȧ : up=Ȧ : lo=ȧ */
  { 0xc8a7 , 0xc8a6 , 0xc8a7 , None , "a" }, /* ȧ : up=Ȧ : lo=ȧ */
  { 0xc8a8 , 0xc8a8 , 0xc8a9 , None , "E" }, /* Ȩ : up=Ȩ : lo=ȩ */
  { 0xc8a9 , 0xc8a8 , 0xc8a9 , None , "e" }, /* ȩ : up=Ȩ : lo=ȩ */
  { 0xc8aa , 0xc8aa , 0xc8ab , None , "O" }, /* Ȫ : up=Ȫ : lo=ȫ */
  { 0xc8ab , 0xc8aa , 0xc8ab , None , "o" }, /* ȫ : up=Ȫ : lo=ȫ */
  { 0xc8ac , 0xc8ac , 0xc8ad , None , "O" }, /* Ȭ : up=Ȭ : lo=ȭ */
  { 0xc8ad , 0xc8ac , 0xc8ad , None , "o" }, /* ȭ : up=Ȭ : lo=ȭ */
  { 0xc8ae , 0xc8ae , 0xc8af , None , "O" }, /* Ȯ : up=Ȯ : lo=ȯ */
  { 0xc8af , 0xc8ae , 0xc8af , None , "o" }, /* ȯ : up=Ȯ : lo=ȯ */
  { 0xc8b0 , 0xc8b0 , 0xc8b1 , None , "O" }, /* Ȱ : up=Ȱ : lo=ȱ */
  { 0xc8b1 , 0xc8b0 , 0xc8b1 , None , "o" }, /* ȱ : up=Ȱ : lo=ȱ */
  { 0xc8b2 , 0xc8b2 , 0xc8b3 , None , "Y" }, /* Ȳ : up=Ȳ : lo=ȳ */
  { 0xc8b3 , 0xc8b2 , 0xc8b3 , None , "y" }, /* ȳ : up=Ȳ : lo=ȳ */
  { 0xc8b4 , 0xc8b4 , 0xc8b4 , None , "l" }, /* ȴ : up=ȴ : lo=ȴ */
  { 0xc8b5 , 0xc8b5 , 0xc8b5 , None , "n" }, /* ȵ : up=ȵ : lo=ȵ */
  { 0xc8b6 , 0xc8b6 , 0xc8b6 , None , "t" }, /* ȶ : up=ȶ : lo=ȶ */
  { 0xc8b7 , 0xc8b7 , 0xc8b7 , None , "j" }, /* ȷ : up=ȷ : lo=ȷ */
  { 0xc8b8 , 0xc8b8 , 0xc8b8 , None , "db" }, /* ȸ : up=ȸ : lo=ȸ */
  { 0xc8b9 , 0xc8b9 , 0xc8b9 , None , "qp" }, /* ȹ : up=ȹ : lo=ȹ */
  { 0xc8ba , 0xc8ba , 0xc8ba , None , "A" }, /* Ⱥ : up=Ⱥ : lo=Ⱥ */
  { 0xc8bb , 0xc8bb , 0xc8bc , None , "C" }, /* Ȼ : up=Ȼ : lo=ȼ */
  { 0xc8bc , 0xc8bb , 0xc8bc , None , "c" }, /* ȼ : up=Ȼ : lo=ȼ */
  { 0xc8bd , 0xc8bd , 0xc69a , None , "L" }, /* Ƚ : up=Ƚ : lo=ƚ */
  { 0xc8be , 0xc8be , 0xc8be , None , "T" }, /* Ⱦ : up=Ⱦ : lo=Ⱦ */
  { 0xc8bf , 0xc8bf , 0xc8bf , None , "s" }, /* ȿ : up=ȿ : lo=ȿ */
};

const character charmap_c9[64] = {
  { 0xc980 , 0xc980 , 0xc980 , None , "z" }, /* ɀ : up=ɀ : lo=ɀ */
  { 0xc981 , 0xc981 , 0xc982 , None , u8"\xc9\x81" }, /* Ɂ : up=Ɂ : lo=ɂ */
  { 0xc982 , 0xc981 , 0xc982 , None , u8"\xc9\x82" }, /* ɂ : up=Ɂ : lo=ɂ */
  { 0xc983 , 0xc983 , 0xc680 , None , "B" }, /* Ƀ : up=Ƀ : lo=ƀ */
  { 0xc984 , 0xc984 , 0xca89 , None , "U" }, /* Ʉ : up=Ʉ : lo=ʉ */
  { 0xc985 , 0xc985 , 0xca8c , None , u8"\xc9\x85" }, /* Ʌ : up=Ʌ : lo=ʌ */
  { 0xc986 , 0xc986 , 0xc987 , None , "E" }, /* Ɇ : up=Ɇ : lo=ɇ */
  { 0xc987 , 0xc986 , 0xc987 , None , "e" }, /* ɇ : up=Ɇ : lo=ɇ */
  { 0xc988 , 0xc988 , 0xc989 , None , "J" }, /* Ɉ : up=Ɉ : lo=ɉ */
  { 0xc989 , 0xc988 , 0xc989 , None , "j" }, /* ɉ : up=Ɉ : lo=ɉ */
  { 0xc98a , 0xc98a , 0xc98b , None , u8"\xc9\x8a" }, /* Ɋ : up=Ɋ : lo=ɋ */
  { 0xc98b , 0xc98a , 0xc98b , None , u8"\xc9\x90" }, /* ɋ : up=Ɋ : lo=ɋ */
  { 0xc98c , 0xc98c , 0xc98d , None , "R" }, /* Ɍ : up=Ɍ : lo=ɍ */
  { 0xc98d , 0xc98c , 0xc98d , None , "r" }, /* ɍ : up=Ɍ : lo=ɍ */
  { 0xc98e , 0xc98e , 0xc98f , None , "Y" }, /* Ɏ : up=Ɏ : lo=ɏ */
  { 0xc98f , 0xc98e , 0xc98f , None , "y" }, /* ɏ : up=Ɏ : lo=ɏ */
  { 0xc990 , 0xc990 , 0xc990 , None , u8"\xc9\x90" }, /* ɐ : up=ɐ : lo=ɐ */
  { 0xc991 , 0xc991 , 0xc991 , None , u8"\xc9\x91" }, /* ɑ : up=ɑ : lo=ɑ */
  { 0xc992 , 0xc992 , 0xc992 , None , u8"\xc9\x92" }, /* ɒ : up=ɒ : lo=ɒ */
  { 0xc993 , 0xc681 , 0xc993 , None , "b" }, /* ɓ : up=Ɓ : lo=ɓ */
  { 0xc994 , 0xc686 , 0xc994 , None , u8"\xc9\x94" }, /* ɔ : up=Ɔ : lo=ɔ */
  { 0xc995 , 0xc995 , 0xc995 , None , "c" }, /* ɕ : up=ɕ : lo=ɕ */
  { 0xc996 , 0xc689 , 0xc996 , None , "d" }, /* ɖ : up=Ɖ : lo=ɖ */
  { 0xc997 , 0xc68a , 0xc997 , None , "D" }, /* ɗ : up=Ɗ : lo=ɗ */
  { 0xc998 , 0xc998 , 0xc998 , None , u8"\xc9\x98" }, /* ɘ : up=ɘ : lo=ɘ */
  { 0xc999 , 0xc68f , 0xc999 , None , u8"\xc9\x99" }, /* ə : up=Ə : lo=ə */
  { 0xc99a , 0xc99a , 0xc99a , None , u8"\xc9\x9a" }, /* ɚ : up=ɚ : lo=ɚ */
  { 0xc99b , 0xc690 , 0xc99b , None , "e" }, /* ɛ : up=Ɛ : lo=ɛ */
  { 0xc99c , 0xc99c , 0xc99c , None , u8"\xc9\x9c" }, /* ɜ : up=ɜ : lo=ɜ */
  { 0xc99d , 0xc99d , 0xc99d , None , u8"\xc9\x9d" }, /* ɝ : up=ɝ : lo=ɝ */
  { 0xc99e , 0xc99e , 0xc99e , None , u8"\xc9\x9e" }, /* ɞ : up=ɞ : lo=ɞ */
  { 0xc99f , 0xc99f , 0xc99f , None , "j" }, /* ɟ : up=ɟ : lo=ɟ */
  { 0xc9a0 , 0xc693 , 0xc9a0 , None , "g" }, /* ɠ : up=Ɠ : lo=ɠ */
  { 0xc9a1 , 0xc9a1 , 0xc9a1 , None , "g" }, /* ɡ : up=ɡ : lo=ɡ */
  { 0xc9a2 , 0xc9a2 , 0xc9a2 , None , "G" }, /* ɢ : up=ɢ : lo=ɢ */
  { 0xc9a3 , 0xc694 , 0xc9a3 , None , u8"\xc9\xa3" }, /* ɣ : up=Ɣ : lo=ɣ */
  { 0xc9a4 , 0xc9a4 , 0xc9a4 , None , u8"\xc9\xa4" }, /* ɤ : up=ɤ : lo=ɤ */
  { 0xc9a5 , 0xc9a5 , 0xc9a5 , None , u8"\xc9\xa5" }, /* ɥ : up=ɥ : lo=ɥ */
  { 0xc9a6 , 0xc9a6 , 0xc9a6 , None , "h" }, /* ɦ : up=ɦ : lo=ɦ */
  { 0xc9a7 , 0xc9a7 , 0xc9a7 , None , "h" }, /* ɧ : up=ɧ : lo=ɧ */
  { 0xc9a8 , 0xc697 , 0xc9a8 , None , "i" }, /* ɨ : up=Ɨ : lo=ɨ */
  { 0xc9a9 , 0xc696 , 0xc9a9 , None , "i" }, /* ɩ : up=Ɩ : lo=ɩ */
  { 0xc9aa , 0xc9aa , 0xc9aa , None , "I" }, /* ɪ : up=ɪ : lo=ɪ */
  { 0xc9ab , 0xc9ab , 0xc9ab , None , "l" }, /* ɫ : up=ɫ : lo=ɫ */
  { 0xc9ac , 0xc9ac , 0xc9ac , None , "l" }, /* ɬ : up=ɬ : lo=ɬ */
  { 0xc9ad , 0xc9ad , 0xc9ad , None , "l" }, /* ɭ : up=ɭ : lo=ɭ */
  { 0xc9ae , 0xc9ae , 0xc9ae , None , u8"\xc9\xae" }, /* ɮ : up=ɮ : lo=ɮ */
  { 0xc9af , 0xc69c , 0xc9af , None , u8"\xc9\xaf" }, /* ɯ : up=Ɯ : lo=ɯ */
  { 0xc9b0 , 0xc9b0 , 0xc9b0 , None , u8"\xc9\xb0" }, /* ɰ : up=ɰ : lo=ɰ */
  { 0xc9b1 , 0xc9b1 , 0xc9b1 , None , u8"\xc9\xb1" }, /* ɱ : up=ɱ : lo=ɱ */
  { 0xc9b2 , 0xc69d , 0xc9b2 , None , "n" }, /* ɲ : up=Ɲ : lo=ɲ */
  { 0xc9b3 , 0xc9b3 , 0xc9b3 , None , "n" }, /* ɳ : up=ɳ : lo=ɳ */
  { 0xc9b4 , 0xc9b4 , 0xc9b4 , None , "N" }, /* ɴ : up=ɴ : lo=ɴ */
  { 0xc9b5 , 0xc69f , 0xc9b5 , None , u8"\xc9\xb5" }, /* ɵ : up=Ɵ : lo=ɵ */
  { 0xc9b6 , 0xc9b6 , 0xc9b6 , None , "OE" }, /* ɶ : up=ɶ : lo=ɶ */
  { 0xc9b7 , 0xc9b7 , 0xc9b7 , None , u8"\xc9\xb7" }, /* ɷ : up=ɷ : lo=ɷ */
  { 0xc9b8 , 0xc9b8 , 0xc9b8 , None , u8"\xc9\xb8" }, /* ɸ : up=ɸ : lo=ɸ */
  { 0xc9b9 , 0xc9b9 , 0xc9b9 , None , u8"\xc9\xb9" }, /* ɹ : up=ɹ : lo=ɹ */
  { 0xc9ba , 0xc9ba , 0xc9ba , None , u8"\xc9\xba" }, /* ɺ : up=ɺ : lo=ɺ */
  { 0xc9bb , 0xc9bb , 0xc9bb , None , u8"\xc9\xbb" }, /* ɻ : up=ɻ : lo=ɻ */
  { 0xc9bc , 0xc9bc , 0xc9bc , None , "r" }, /* ɼ : up=ɼ : lo=ɼ */
  { 0xc9bd , 0xc9bd , 0xc9bd , None , "r" }, /* ɽ : up=ɽ : lo=ɽ */
  { 0xc9be , 0xc9be , 0xc9be , None , "r" }, /* ɾ : up=ɾ : lo=ɾ */
  { 0xc9bf , 0xc9bf , 0xc9bf , None , u8"\xc9\xbf" }, /* ɿ : up=ɿ : lo=ɿ */
};

const character charmap_ca[64] = {
  { 0xca80 , 0xc6a6 , 0xca80 , None , "R" }, /* ʀ : up=Ʀ : lo=ʀ */
  { 0xca81 , 0xca81 , 0xca81 , None , u8"\xca\x81" }, /* ʁ : up=ʁ : lo=ʁ */
  { 0xca82 , 0xca82 , 0xca82 , None , "s" }, /* ʂ : up=ʂ : lo=ʂ */
  { 0xca83 , 0xc6a9 , 0xca83 , None , u8"\xca\x83" }, /* ʃ : up=Ʃ : lo=ʃ */
  { 0xca84 , 0xca84 , 0xca84 , None , u8"\xca\x84" }, /* ʄ : up=ʄ : lo=ʄ */
  { 0xca85 , 0xca85 , 0xca85 , None , u8"\xca\x85" }, /* ʅ : up=ʅ : lo=ʅ */
  { 0xca86 , 0xca86 , 0xca86 , None , u8"\xca\x86" }, /* ʆ : up=ʆ : lo=ʆ */
  { 0xca87 , 0xca87 , 0xca87 , None , u8"\xca\x87" }, /* ʇ : up=ʇ : lo=ʇ */
  { 0xca88 , 0xc6ae , 0xca88 , None , "t" }, /* ʈ : up=Ʈ : lo=ʈ */
  { 0xca89 , 0xc984 , 0xca89 , None , "u" }, /* ʉ : up=Ʉ : lo=ʉ */
  { 0xca8a , 0xc6b1 , 0xca8a , None , u8"\xca\x8a" }, /* ʊ : up=Ʊ : lo=ʊ */
  { 0xca8b , 0xc6b2 , 0xca8b , None , "v" }, /* ʋ : up=Ʋ : lo=ʋ */
  { 0xca8c , 0xc985 , 0xca8c , None , u8"\xca\x8c" }, /* ʌ : up=Ʌ : lo=ʌ */
  { 0xca8d , 0xca8d , 0xca8d , None , u8"\xca\x8d" }, /* ʍ : up=ʍ : lo=ʍ */
  { 0xca8e , 0xca8e , 0xca8e , None , u8"\xca\x8e" }, /* ʎ : up=ʎ : lo=ʎ */
  { 0xca8f , 0xca8f , 0xca8f , None , "Y" }, /* ʏ : up=ʏ : lo=ʏ */
  { 0xca90 , 0xca90 , 0xca90 , None , "z" }, /* ʐ : up=ʐ : lo=ʐ */
  { 0xca91 , 0xca91 , 0xca91 , None , "z" }, /* ʑ : up=ʑ : lo=ʑ */
  { 0xca92 , 0xc6b7 , 0xca92 , None , u8"\xca\x92" }, /* ʒ : up=Ʒ : lo=ʒ */
  { 0xca93 , 0xca93 , 0xca93 , None , u8"\xca\x93" }, /* ʓ : up=ʓ : lo=ʓ */
  { 0xca94 , 0xca94 , 0xca94 , None , u8"\xca\x94" }, /* ʔ : up=ʔ : lo=ʔ */
  { 0xca95 , 0xca95 , 0xca95 , None , u8"\xca\x95" }, /* ʕ : up=ʕ : lo=ʕ */
  { 0xca96 , 0xca96 , 0xca96 , None , u8"\xca\x96" }, /* ʖ : up=ʖ : lo=ʖ */
  { 0xca97 , 0xca97 , 0xca97 , None , u8"\xca\x97" }, /* ʗ : up=ʗ : lo=ʗ */
  { 0xca98 , 0xca98 , 0xca98 , None , u8"\xca\x98" }, /* ʘ : up=ʘ : lo=ʘ */
  { 0xca99 , 0xca99 , 0xca99 , None , "B" }, /* ʙ : up=ʙ : lo=ʙ */
  { 0xca9a , 0xca9a , 0xca9a , None , u8"\xca\x9a" }, /* ʚ : up=ʚ : lo=ʚ */
  { 0xca9b , 0xca9b , 0xca9b , None , "G" }, /* ʛ : up=ʛ : lo=ʛ */
  { 0xca9c , 0xca9c , 0xca9c , None , "H" }, /* ʜ : up=ʜ : lo=ʜ */
  { 0xca9d , 0xca9d , 0xca9d , None , "j" }, /* ʝ : up=ʝ : lo=ʝ */
  { 0xca9e , 0xca9e , 0xca9e , None , u8"\xca\x9e" }, /* ʞ : up=ʞ : lo=ʞ */
  { 0xca9f , 0xca9f , 0xca9f , None , "L" }, /* ʟ : up=ʟ : lo=ʟ */
  { 0xcaa0 , 0xcaa0 , 0xcaa0 , None , "q" }, /* ʠ : up=ʠ : lo=ʠ */
  { 0xcaa1 , 0xcaa1 , 0xcaa1 , None , u8"\xca\xa1" }, /* ʡ : up=ʡ : lo=ʡ */
  { 0xcaa2 , 0xcaa2 , 0xcaa2 , None , u8"\xca\xa2" }, /* ʢ : up=ʢ : lo=ʢ */
  { 0xcaa3 , 0xcaa3 , 0xcaa3 , None , "dz" }, /* ʣ : up=ʣ : lo=ʣ */
  { 0xcaa4 , 0xcaa4 , 0xcaa4 , None , u8"\xca\xa4" }, /* ʤ : up=ʤ : lo=ʤ */
  { 0xcaa5 , 0xcaa5 , 0xcaa5 , None , "dz" }, /* ʥ : up=ʥ : lo=ʥ */
  { 0xcaa6 , 0xcaa6 , 0xcaa6 , None , "ts" }, /* ʦ : up=ʦ : lo=ʦ */
  { 0xcaa7 , 0xcaa7 , 0xcaa7 , None , u8"\xca\xa7" }, /* ʧ : up=ʧ : lo=ʧ */
  { 0xcaa8 , 0xcaa8 , 0xcaa8 , None , u8"\xca\xa8" }, /* ʨ : up=ʨ : lo=ʨ */
  { 0xcaa9 , 0xcaa9 , 0xcaa9 , None , u8"\xca\xa9" }, /* ʩ : up=ʩ : lo=ʩ */
  { 0xcaaa , 0xcaaa , 0xcaaa , None , "ls" }, /* ʪ : up=ʪ : lo=ʪ */
  { 0xcaab , 0xcaab , 0xcaab , None , "lz" }, /* ʫ : up=ʫ : lo=ʫ */
  { 0xcaac , 0xcaac , 0xcaac , None , u8"\xca\xac" }, /* ʬ : up=ʬ : lo=ʬ */
  { 0xcaad , 0xcaad , 0xcaad , None , u8"\xca\xad" }, /* ʭ : up=ʭ : lo=ʭ */
  { 0xcaae , 0xcaae , 0xcaae , None , u8"\xca\xae" }, /* ʮ : up=ʮ : lo=ʮ */
  { 0xcaaf , 0xcaaf , 0xcaaf , None , u8"\xca\xaf" }, /* ʯ : up=ʯ : lo=ʯ */
  { 0xcab0 , 0xcab0 , 0xcab0 , IsModifier , "h" }, /* ʰ : up=ʰ : lo=ʰ */
  { 0xcab1 , 0xcab1 , 0xcab1 , IsModifier , "h" }, /* ʱ : up=ʱ : lo=ʱ */
  { 0xcab2 , 0xcab2 , 0xcab2 , IsModifier , "j" }, /* ʲ : up=ʲ : lo=ʲ */
  { 0xcab3 , 0xcab3 , 0xcab3 , IsModifier , "r" }, /* ʳ : up=ʳ : lo=ʳ */
  { 0xcab4 , 0xcab4 , 0xcab4 , IsModifier , u8"\xca\xb4" }, /* ʴ : up=ʴ : lo=ʴ */
  { 0xcab5 , 0xcab5 , 0xcab5 , IsModifier , u8"\xca\xb5" }, /* ʵ : up=ʵ : lo=ʵ */
  { 0xcab6 , 0xcab6 , 0xcab6 , IsModifier , u8"\xca\xb6" }, /* ʶ : up=ʶ : lo=ʶ */
  { 0xcab7 , 0xcab7 , 0xcab7 , IsModifier , "w" }, /* ʷ : up=ʷ : lo=ʷ */
  { 0xcab8 , 0xcab8 , 0xcab8 , IsModifier , "y" }, /* ʸ : up=ʸ : lo=ʸ */
  { 0xcab9 , 0xcab9 , 0xcab9 , IsModifier , "r" }, /* ʹ : up=ʹ : lo=ʹ */
  { 0xcaba , 0xcaba , 0xcaba , IsModifier , u8"\xca\xba" }, /* ʺ : up=ʺ : lo=ʺ */
  { 0xcabb , 0xcabb , 0xcabb , IsModifier , u8"\xca\xbb" }, /* ʻ : up=ʻ : lo=ʻ */
  { 0xcabc , 0xcabc , 0xcabc , IsModifier , u8"\xca\xbc" }, /* ʼ : up=ʼ : lo=ʼ */
  { 0xcabd , 0xcabd , 0xcabd , IsModifier , u8"\xca\xbd" }, /* ʽ : up=ʽ : lo=ʽ */
  { 0xcabe , 0xcabe , 0xcabe , IsModifier , u8"\xca\xbe" }, /* ʾ : up=ʾ : lo=ʾ */
  { 0xcabf , 0xcabf , 0xcabf , IsModifier , u8"\xca\xbf" }, /* ʿ : up=ʿ : lo=ʿ */
};

const character charmap_cb[64] = {
  { 0xcb80 , 0xcb80 , 0xcb80 , IsModifier , u8"\xcb\x80" }, /* ˀ : up=ˀ : lo=ˀ */
  { 0xcb81 , 0xcb81 , 0xcb81 , IsModifier , u8"\xcb\x81" }, /* ˁ : up=ˁ : lo=ˁ */
  { 0xcb82 , 0xcb82 , 0xcb82 , IsModifier , u8"\xcb\x82" }, /* ˂ : up=˂ : lo=˂ */
  { 0xcb83 , 0xcb83 , 0xcb83 , IsModifier , u8"\xcb\x83" }, /* ˃ : up=˃ : lo=˃ */
  { 0xcb84 , 0xcb84 , 0xcb84 , IsModifier , u8"\xcb\x84" }, /* ˄ : up=˄ : lo=˄ */
  { 0xcb85 , 0xcb85 , 0xcb85 , IsModifier , u8"\xcb\x85" }, /* ˅ : up=˅ : lo=˅ */
  { 0xcb86 , 0xcb86 , 0xcb86 , IsModifier , u8"\xcb\x86" }, /* ˆ : up=ˆ : lo=ˆ */
  { 0xcb87 , 0xcb87 , 0xcb87 , IsModifier , u8"\xcb\x87" }, /* ˇ : up=ˇ : lo=ˇ */
  { 0xcb88 , 0xcb88 , 0xcb88 , IsModifier , u8"\xcb\x88" }, /* ˈ : up=ˈ : lo=ˈ */
  { 0xcb89 , 0xcb89 , 0xcb89 , IsModifier , u8"\xcb\x89" }, /* ˉ : up=ˉ : lo=ˉ */
  { 0xcb8a , 0xcb8a , 0xcb8a , IsModifier , u8"\xcb\x8a" }, /* ˊ : up=ˊ : lo=ˊ */
  { 0xcb8b , 0xcb8b , 0xcb8b , IsModifier , u8"\xcb\x8b" }, /* ˋ : up=ˋ : lo=ˋ */
  { 0xcb8c , 0xcb8c , 0xcb8c , IsModifier , u8"\xcb\x8c" }, /* ˌ : up=ˌ : lo=ˌ */
  { 0xcb8d , 0xcb8d , 0xcb8d , IsModifier , u8"\xcb\x8d" }, /* ˍ : up=ˍ : lo=ˍ */
  { 0xcb8e , 0xcb8e , 0xcb8e , IsModifier , u8"\xcb\x8e" }, /* ˎ : up=ˎ : lo=ˎ */
  { 0xcb8f , 0xcb8f , 0xcb8f , IsModifier , u8"\xcb\x8f" }, /* ˏ : up=ˏ : lo=ˏ */
  { 0xcb90 , 0xcb90 , 0xcb90 , IsModifier , u8"\xcb\x90" }, /* ː : up=ː : lo=ː */
  { 0xcb91 , 0xcb91 , 0xcb91 , IsModifier , u8"\xcb\x91" }, /* ˑ : up=ˑ : lo=ˑ */
  { 0xcb92 , 0xcb92 , 0xcb92 , IsModifier , u8"\xcb\x92" }, /* ˒ : up=˒ : lo=˒ */
  { 0xcb93 , 0xcb93 , 0xcb93 , IsModifier , u8"\xcb\x93" }, /* ˓ : up=˓ : lo=˓ */
  { 0xcb94 , 0xcb94 , 0xcb94 , IsModifier , u8"\xcb\x94" }, /* ˔ : up=˔ : lo=˔ */
  { 0xcb95 , 0xcb95 , 0xcb95 , IsModifier , u8"\xcb\x95" }, /* ˕ : up=˕ : lo=˕ */
  { 0xcb96 , 0xcb96 , 0xcb96 , IsModifier , u8"\xcb\x96" }, /* ˖ : up=˖ : lo=˖ */
  { 0xcb97 , 0xcb97 , 0xcb97 , IsModifier , u8"\xcb\x97" }, /* ˗ : up=˗ : lo=˗ */
  { 0xcb98 , 0xcb98 , 0xcb98 , IsModifier , u8"\xcb\x98" }, /* ˘ : up=˘ : lo=˘ */
  { 0xcb99 , 0xcb99 , 0xcb99 , IsModifier , u8"\xcb\x99" }, /* ˙ : up=˙ : lo=˙ */
  { 0xcb9a , 0xcb9a , 0xcb9a , IsModifier , u8"\xcb\x9a" }, /* ˚ : up=˚ : lo=˚ */
  { 0xcb9b , 0xcb9b , 0xcb9b , IsModifier , u8"\xcb\x9b" }, /* ˛ : up=˛ : lo=˛ */
  { 0xcb9c , 0xcb9c , 0xcb9c , IsModifier , u8"\xcb\x9c" }, /* ˜ : up=˜ : lo=˜ */
  { 0xcb9d , 0xcb9d , 0xcb9d , IsModifier , u8"\xcb\x9d" }, /* ˝ : up=˝ : lo=˝ */
  { 0xcb9e , 0xcb9e , 0xcb9e , IsModifier , u8"\xcb\x9e" }, /* ˞ : up=˞ : lo=˞ */
  { 0xcb9f , 0xcb9f , 0xcb9f , IsModifier , u8"\xcb\x9f" }, /* ˟ : up=˟ : lo=˟ */
  { 0xcba0 , 0xcba0 , 0xcba0 , IsModifier , u8"\xcb\xa0" }, /* ˠ : up=ˠ : lo=ˠ */
  { 0xcba1 , 0xcba1 , 0xcba1 , IsModifier , "l" }, /* ˡ : up=ˡ : lo=ˡ */
  { 0xcba2 , 0xcba2 , 0xcba2 , IsModifier , "s" }, /* ˢ : up=ˢ : lo=ˢ */
  { 0xcba3 , 0xcba3 , 0xcba3 , IsModifier , "x" }, /* ˣ : up=ˣ : lo=ˣ */
  { 0xcba4 , 0xcba4 , 0xcba4 , IsModifier , u8"\xcb\xa4" }, /* ˤ : up=ˤ : lo=ˤ */
  { 0xcba5 , 0xcba5 , 0xcba5 , IsModifier , u8"\xcb\xa5" }, /* ˥ : up=˥ : lo=˥ */
  { 0xcba6 , 0xcba6 , 0xcba6 , IsModifier , u8"\xcb\xa6" }, /* ˦ : up=˦ : lo=˦ */
  { 0xcba7 , 0xcba7 , 0xcba7 , IsModifier , u8"\xcb\xa7" }, /* ˧ : up=˧ : lo=˧ */
  { 0xcba8 , 0xcba8 , 0xcba8 , IsModifier , u8"\xcb\xa8" }, /* ˨ : up=˨ : lo=˨ */
  { 0xcba9 , 0xcba9 , 0xcba9 , IsModifier , u8"\xcb\xa9" }, /* ˩ : up=˩ : lo=˩ */
  { 0xcbaa , 0xcbaa , 0xcbaa , IsModifier , u8"\xcb\xaa" }, /* ˪ : up=˪ : lo=˪ */
  { 0xcbab , 0xcbab , 0xcbab , IsModifier , u8"\xcb\xab" }, /* ˫ : up=˫ : lo=˫ */
  { 0xcbac , 0xcbac , 0xcbac , IsModifier , u8"\xcb\xac" }, /* ˬ : up=ˬ : lo=ˬ */
  { 0xcbad , 0xcbad , 0xcbad , IsModifier , u8"\xcb\xad" }, /* ˭ : up=˭ : lo=˭ */
  { 0xcbae , 0xcbae , 0xcbae , IsModifier , u8"\xcb\xae" }, /* ˮ : up=ˮ : lo=ˮ */
  { 0xcbaf , 0xcbaf , 0xcbaf , IsModifier , u8"\xcb\xaf" }, /* ˯ : up=˯ : lo=˯ */
  { 0xcbb0 , 0xcbb0 , 0xcbb0 , IsModifier , u8"\xcb\xb0" }, /* ˰ : up=˰ : lo=˰ */
  { 0xcbb1 , 0xcbb1 , 0xcbb1 , IsModifier , u8"\xcb\xb1" }, /* ˱ : up=˱ : lo=˱ */
  { 0xcbb2 , 0xcbb2 , 0xcbb2 , IsModifier , u8"\xcb\xb2" }, /* ˲ : up=˲ : lo=˲ */
  { 0xcbb3 , 0xcbb3 , 0xcbb3 , IsModifier , u8"\xcb\xb3" }, /* ˳ : up=˳ : lo=˳ */
  { 0xcbb4 , 0xcbb4 , 0xcbb4 , IsModifier , u8"\xcb\xb4" }, /* ˴ : up=˴ : lo=˴ */
  { 0xcbb5 , 0xcbb5 , 0xcbb5 , IsModifier , u8"\xcb\xb5" }, /* ˵ : up=˵ : lo=˵ */
  { 0xcbb6 , 0xcbb6 , 0xcbb6 , IsModifier , u8"\xcb\xb6" }, /* ˶ : up=˶ : lo=˶ */
  { 0xcbb7 , 0xcbb7 , 0xcbb7 , IsModifier , u8"\xcb\xb7" }, /* ˷ : up=˷ : lo=˷ */
  { 0xcbb8 , 0xcbb8 , 0xcbb8 , IsModifier , u8"\xcb\xb8" }, /* ˸ : up=˸ : lo=˸ */
  { 0xcbb9 , 0xcbb9 , 0xcbb9 , IsModifier , u8"\xcb\xb9" }, /* ˹ : up=˹ : lo=˹ */
  { 0xcbba , 0xcbba , 0xcbba , IsModifier , u8"\xcb\xba" }, /* ˺ : up=˺ : lo=˺ */
  { 0xcbbb , 0xcbbb , 0xcbbb , IsModifier , u8"\xcb\xbb" }, /* ˻ : up=˻ : lo=˻ */
  { 0xcbbc , 0xcbbc , 0xcbbc , IsModifier , u8"\xcb\xbc" }, /* ˼ : up=˼ : lo=˼ */
  { 0xcbbd , 0xcbbd , 0xcbbd , IsModifier , u8"\xcb\xbd" }, /* ˽ : up=˽ : lo=˽ */
  { 0xcbbe , 0xcbbe , 0xcbbe , IsModifier , u8"\xcb\xbe" }, /* ˾ : up=˾ : lo=˾ */
  { 0xcbbf , 0xcbbf , 0xcbbf , IsModifier , u8"\xcb\xbf" }, /* ˿ : up=˿ : lo=˿ */
};

const character charmap_cc[64] = {
  { 0xcc80 , 0xcc80 , 0xcc80 , IsDiacritic , u8"\xcc\x80" },
  { 0xcc81 , 0xcc81 , 0xcc81 , IsDiacritic , u8"\xcc\x81" },
  { 0xcc82 , 0xcc82 , 0xcc82 , IsDiacritic , u8"\xcc\x82" },
  { 0xcc83 , 0xcc83 , 0xcc83 , IsDiacritic , u8"\xcc\x83" },
  { 0xcc84 , 0xcc84 , 0xcc84 , IsDiacritic , u8"\xcc\x84" },
  { 0xcc85 , 0xcc85 , 0xcc85 , IsDiacritic , u8"\xcc\x85" },
  { 0xcc86 , 0xcc86 , 0xcc86 , IsDiacritic , u8"\xcc\x86" },
  { 0xcc87 , 0xcc87 , 0xcc87 , IsDiacritic , u8"\xcc\x87" },
  { 0xcc88 , 0xcc88 , 0xcc88 , IsDiacritic , u8"\xcc\x88" },
  { 0xcc89 , 0xcc89 , 0xcc89 , IsDiacritic , u8"\xcc\x89" },
  { 0xcc8a , 0xcc8a , 0xcc8a , IsDiacritic , u8"\xcc\x8a" },
  { 0xcc8b , 0xcc8b , 0xcc8b , IsDiacritic , u8"\xcc\x8b" },
  { 0xcc8c , 0xcc8c , 0xcc8c , IsDiacritic , u8"\xcc\x8c" },
  { 0xcc8d , 0xcc8d , 0xcc8d , IsDiacritic , u8"\xcc\x8d" },
  { 0xcc8e , 0xcc8e , 0xcc8e , IsDiacritic , u8"\xcc\x8e" },
  { 0xcc8f , 0xcc8f , 0xcc8f , IsDiacritic , u8"\xcc\x8f" },
  { 0xcc90 , 0xcc90 , 0xcc90 , IsDiacritic , u8"\xcc\x90" },
  { 0xcc91 , 0xcc91 , 0xcc91 , IsDiacritic , u8"\xcc\x91" },
  { 0xcc92 , 0xcc92 , 0xcc92 , IsDiacritic , u8"\xcc\x92" },
  { 0xcc93 , 0xcc93 , 0xcc93 , IsDiacritic , u8"\xcc\x93" },
  { 0xcc94 , 0xcc94 , 0xcc94 , IsDiacritic , u8"\xcc\x94" },
  { 0xcc95 , 0xcc95 , 0xcc95 , IsDiacritic , u8"\xcc\x95" },
  { 0xcc96 , 0xcc96 , 0xcc96 , IsDiacritic , u8"\xcc\x96" },
  { 0xcc97 , 0xcc97 , 0xcc97 , IsDiacritic , u8"\xcc\x97" },
  { 0xcc98 , 0xcc98 , 0xcc98 , IsDiacritic , u8"\xcc\x98" },
  { 0xcc99 , 0xcc99 , 0xcc99 , IsDiacritic , u8"\xcc\x99" },
  { 0xcc9a , 0xcc9a , 0xcc9a , IsDiacritic , u8"\xcc\x9a" },
  { 0xcc9b , 0xcc9b , 0xcc9b , IsDiacritic , u8"\xcc\x9b" },
  { 0xcc9c , 0xcc9c , 0xcc9c , IsDiacritic , u8"\xcc\x9c" },
  { 0xcc9d , 0xcc9d , 0xcc9d , IsDiacritic , u8"\xcc\x9d" },
  { 0xcc9e , 0xcc9e , 0xcc9e , IsDiacritic , u8"\xcc\x9e" },
  { 0xcc9f , 0xcc9f , 0xcc9f , IsDiacritic , u8"\xcc\x9f" },
  { 0xcca0 , 0xcca0 , 0xcca0 , IsDiacritic , u8"\xcc\xa0" },
  { 0xcca1 , 0xcca1 , 0xcca1 , IsDiacritic , u8"\xcc\xa1" },
  { 0xcca2 , 0xcca2 , 0xcca2 , IsDiacritic , u8"\xcc\xa2" },
  { 0xcca3 , 0xcca3 , 0xcca3 , IsDiacritic , u8"\xcc\xa3" },
  { 0xcca4 , 0xcca4 , 0xcca4 , IsDiacritic , u8"\xcc\xa4" },
  { 0xcca5 , 0xcca5 , 0xcca5 , IsDiacritic , u8"\xcc\xa5" },
  { 0xcca6 , 0xcca6 , 0xcca6 , IsDiacritic , u8"\xcc\xa6" },
  { 0xcca7 , 0xcca7 , 0xcca7 , IsDiacritic , u8"\xcc\xa7" },
  { 0xcca8 , 0xcca8 , 0xcca8 , IsDiacritic , u8"\xcc\xa8" },
  { 0xcca9 , 0xcca9 , 0xcca9 , IsDiacritic , u8"\xcc\xa9" },
  { 0xccaa , 0xccaa , 0xccaa , IsDiacritic , u8"\xcc\xaa" },
  { 0xccab , 0xccab , 0xccab , IsDiacritic , u8"\xcc\xab" },
  { 0xccac , 0xccac , 0xccac , IsDiacritic , u8"\xcc\xac" },
  { 0xccad , 0xccad , 0xccad , IsDiacritic , u8"\xcc\xad" },
  { 0xccae , 0xccae , 0xccae , IsDiacritic , u8"\xcc\xae" },
  { 0xccaf , 0xccaf , 0xccaf , IsDiacritic , u8"\xcc\xaf" },
  { 0xccb0 , 0xccb0 , 0xccb0 , IsDiacritic , u8"\xcc\xb0" },
  { 0xccb1 , 0xccb1 , 0xccb1 , IsDiacritic , u8"\xcc\xb1" },
  { 0xccb2 , 0xccb2 , 0xccb2 , IsDiacritic , u8"\xcc\xb2" },
  { 0xccb3 , 0xccb3 , 0xccb3 , IsDiacritic , u8"\xcc\xb3" },
  { 0xccb4 , 0xccb4 , 0xccb4 , IsDiacritic , u8"\xcc\xb4" },
  { 0xccb5 , 0xccb5 , 0xccb5 , IsDiacritic , u8"\xcc\xb5" },
  { 0xccb6 , 0xccb6 , 0xccb6 , IsDiacritic , u8"\xcc\xb6" },
  { 0xccb7 , 0xccb7 , 0xccb7 , IsDiacritic , u8"\xcc\xb7" },
  { 0xccb8 , 0xccb8 , 0xccb8 , IsDiacritic , u8"\xcc\xb8" },
  { 0xccb9 , 0xccb9 , 0xccb9 , IsDiacritic , u8"\xcc\xb9" },
  { 0xccba , 0xccba , 0xccba , IsDiacritic , u8"\xcc\xba" },
  { 0xccbb , 0xccbb , 0xccbb , IsDiacritic , u8"\xcc\xbb" },
  { 0xccbc , 0xccbc , 0xccbc , IsDiacritic , u8"\xcc\xbc" },
  { 0xccbd , 0xccbd , 0xccbd , IsDiacritic , u8"\xcc\xbd" },
  { 0xccbe , 0xccbe , 0xccbe , IsDiacritic , u8"\xcc\xbe" },
  { 0xccbf , 0xccbf , 0xccbf , IsDiacritic , u8"\xcc\xbf" },
};

const character charmap_cd[64] = {
  { 0xcd80 , 0xcd80 , 0xcd80 , IsDiacritic , u8"\xcd\x80" },
  { 0xcd81 , 0xcd81 , 0xcd81 , IsDiacritic , u8"\xcd\x81" },
  { 0xcd82 , 0xcd82 , 0xcd82 , IsDiacritic , u8"\xcd\x82" },
  { 0xcd83 , 0xcd83 , 0xcd83 , IsDiacritic , u8"\xcd\x83" },
  { 0xcd84 , 0xcd84 , 0xcd84 , IsDiacritic , u8"\xcd\x84" },
  { 0xcd85 , 0xcd85 , 0xcd85 , IsDiacritic , u8"\xcd\x85" },
  { 0xcd86 , 0xcd86 , 0xcd86 , IsDiacritic , u8"\xcd\x86" },
  { 0xcd87 , 0xcd87 , 0xcd87 , IsDiacritic , u8"\xcd\x87" },
  { 0xcd88 , 0xcd88 , 0xcd88 , IsDiacritic , u8"\xcd\x88" },
  { 0xcd89 , 0xcd89 , 0xcd89 , IsDiacritic , u8"\xcd\x89" },
  { 0xcd8a , 0xcd8a , 0xcd8a , IsDiacritic , u8"\xcd\x8a" },
  { 0xcd8b , 0xcd8b , 0xcd8b , IsDiacritic , u8"\xcd\x8b" },
  { 0xcd8c , 0xcd8c , 0xcd8c , IsDiacritic , u8"\xcd\x8c" },
  { 0xcd8d , 0xcd8d , 0xcd8d , IsDiacritic , u8"\xcd\x8d" },
  { 0xcd8e , 0xcd8e , 0xcd8e , IsDiacritic , u8"\xcd\x8e" },
  { 0xcd8f , 0xcd8f , 0xcd8f , IsDiacritic , u8"\xcd\x8f" },
  { 0xcd90 , 0xcd90 , 0xcd90 , IsDiacritic , u8"\xcd\x90" },
  { 0xcd91 , 0xcd91 , 0xcd91 , IsDiacritic , u8"\xcd\x91" },
  { 0xcd92 , 0xcd92 , 0xcd92 , IsDiacritic , u8"\xcd\x92" },
  { 0xcd93 , 0xcd93 , 0xcd93 , IsDiacritic , u8"\xcd\x93" },
  { 0xcd94 , 0xcd94 , 0xcd94 , IsDiacritic , u8"\xcd\x94" },
  { 0xcd95 , 0xcd95 , 0xcd95 , IsDiacritic , u8"\xcd\x95" },
  { 0xcd96 , 0xcd96 , 0xcd96 , IsDiacritic , u8"\xcd\x96" },
  { 0xcd97 , 0xcd97 , 0xcd97 , IsDiacritic , u8"\xcd\x97" },
  { 0xcd98 , 0xcd98 , 0xcd98 , IsDiacritic , u8"\xcd\x98" },
  { 0xcd99 , 0xcd99 , 0xcd99 , IsDiacritic , u8"\xcd\x99" },
  { 0xcd9a , 0xcd9a , 0xcd9a , IsDiacritic , u8"\xcd\x9a" },
  { 0xcd9b , 0xcd9b , 0xcd9b , IsDiacritic , u8"\xcd\x9b" },
  { 0xcd9c , 0xcd9c , 0xcd9c , IsDiacritic , u8"\xcd\x9c" },
  { 0xcd9d , 0xcd9d , 0xcd9d , IsDiacritic , u8"\xcd\x9d" },
  { 0xcd9e , 0xcd9e , 0xcd9e , IsDiacritic , u8"\xcd\x9e" },
  { 0xcd9f , 0xcd9f , 0xcd9f , IsDiacritic , u8"\xcd\x9f" },
  { 0xcda0 , 0xcda0 , 0xcda0 , IsDiacritic , u8"\xcd\xa0" },
  { 0xcda1 , 0xcda1 , 0xcda1 , IsDiacritic , u8"\xcd\xa1" },
  { 0xcda2 , 0xcda2 , 0xcda2 , IsDiacritic , u8"\xcd\xa2" },
  { 0xcda3 , 0xcda3 , 0xcda3 , IsDiacritic , u8"\xcd\xa3" },
  { 0xcda4 , 0xcda4 , 0xcda4 , IsDiacritic , u8"\xcd\xa4" },
  { 0xcda5 , 0xcda5 , 0xcda5 , IsDiacritic , u8"\xcd\xa5" },
  { 0xcda6 , 0xcda6 , 0xcda6 , IsDiacritic , u8"\xcd\xa6" },
  { 0xcda7 , 0xcda7 , 0xcda7 , IsDiacritic , u8"\xcd\xa7" },
  { 0xcda8 , 0xcda8 , 0xcda8 , IsDiacritic , u8"\xcd\xa8" },
  { 0xcda9 , 0xcda9 , 0xcda9 , IsDiacritic , u8"\xcd\xa9" },
  { 0xcdaa , 0xcdaa , 0xcdaa , IsDiacritic , u8"\xcd\xaa" },
  { 0xcdab , 0xcdab , 0xcdab , IsDiacritic , u8"\xcd\xab" },
  { 0xcdac , 0xcdac , 0xcdac , IsDiacritic , u8"\xcd\xac" },
  { 0xcdad , 0xcdad , 0xcdad , IsDiacritic , u8"\xcd\xad" },
  { 0xcdae , 0xcdae , 0xcdae , IsDiacritic , u8"\xcd\xae" },
  { 0xcdaf , 0xcdaf , 0xcdaf , IsDiacritic , u8"\xcd\xaf" },
  { 0xcdb0 , 0xcdb0 , 0xcdb1 , None , u8"\xcd\xb0" }, /* Ͱ : up=Ͱ : lo=ͱ */
  { 0xcdb1 , 0xcdb0 , 0xcdb1 , None , u8"\xcd\xb1" }, /* ͱ : up=Ͱ : lo=ͱ */
  { 0xcdb2 , 0xcdb2 , 0xcdb3 , None , u8"\xcd\xb2" }, /* Ͳ : up=Ͳ : lo=ͳ */
  { 0xcdb3 , 0xcdb2 , 0xcdb3 , None , u8"\xcd\xb3" }, /* ͳ : up=Ͳ : lo=ͳ */
  { 0xcdb4 , 0xcdb4 , 0xcdb4 , None , u8"\xcd\xb4" },
  { 0xcdb5 , 0xcdb5 , 0xcdb5 , None , u8"\xcd\xb5" },
  { 0xcdb6 , 0xcdb6 , 0xcdb7 , None , u8"\xcd\xb6" }, /* Ͷ : up=Ͷ : lo=ͷ */
  { 0xcdb7 , 0xcdb6 , 0xcdb7 , None , u8"\xcd\xb7" }, /* ͷ : up=Ͷ : lo=ͷ */
  { 0xcdb8 , 0xcdb8 , 0xcdb8 , None , u8"\xcd\xb8" },
  { 0xcdb9 , 0xcdb9 , 0xcdb9 , None , u8"\xcd\xb9" },
  { 0xcdba , 0xcdba , 0xcdba , None , u8"\xcd\xba" },
  { 0xcdbb , 0xcfbd , 0xcdbb , None , u8"\xcd\xbb" }, /* ͻ : up=Ͻ : lo=ͻ */
  { 0xcdbc , 0xcfbe , 0xcdbc , None , u8"\xcd\xbc" }, /* ͼ : up=Ͼ : lo=ͼ */
  { 0xcdbd , 0xcfbf , 0xcdbd , None , u8"\xcd\xbd" }, /* ͽ : up=Ͽ : lo=ͽ */
  { 0xcdbe , 0xcdbe , 0xcdbe , None , u8"\xcd\xbe" },
  { 0xcdbf , 0xcdbf , 0xcfb3 , None , u8"\xcd\xbf" }, /* Ϳ : up=Ϳ : lo=ϳ */
};

const character charmap_ce[64] = {
  { 0xce80 , 0xce80 , 0xce80 , None , u8"\xce\x80" },
  { 0xce81 , 0xce81 , 0xce81 , None , u8"\xce\x81" },
  { 0xce82 , 0xce82 , 0xce82 , None , u8"\xce\x82" },
  { 0xce83 , 0xce83 , 0xce83 , None , u8"\xce\x83" },
  { 0xce84 , 0xce84 , 0xce84 , None , u8"\xce\x84" },
  { 0xce85 , 0xce85 , 0xce85 , None , u8"\xce\x85" },
  { 0xce86 , 0xce86 , 0xceac , None , u8"\xce\x86" }, /* Ά : up=Ά : lo=ά */
  { 0xce87 , 0xce87 , 0xce87 , None , u8"\xce\x87" }, /* · : up=· : lo=· */
  { 0xce88 , 0xce88 , 0xcead , None , u8"\xce\x88" }, /* Έ : up=Έ : lo=έ */
  { 0xce89 , 0xce89 , 0xceae , None , u8"\xce\x89" }, /* Ή : up=Ή : lo=ή */
  { 0xce8a , 0xce8a , 0xceaf , None , u8"\xce\x8a" }, /* Ί : up=Ί : lo=ί */
  { 0xce8b , 0xce8b , 0xce8b , None , u8"\xce\x8b" },
  { 0xce8c , 0xce8c , 0xcf8c , None , u8"\xce\x8c" }, /* Ό : up=Ό : lo=ό */
  { 0xce8d , 0xce8d , 0xce8d , None , u8"\xce\x8d" },
  { 0xce8e , 0xce8e , 0xcf8d , None , u8"\xce\x8e" }, /* Ύ : up=Ύ : lo=ύ */
  { 0xce8f , 0xce8f , 0xcf8e , None , u8"\xce\x8f" }, /* Ώ : up=Ώ : lo=ώ */
  { 0xce90 , 0xce90 , 0xce90 , None , u8"\xce\x90" }, /* ΐ : up=ΐ : lo=ΐ */
  { 0xce91 , 0xce91 , 0xceb1 , None , u8"\xce\x91" }, /* Α : up=Α : lo=α */
  { 0xce92 , 0xce92 , 0xceb2 , None , u8"\xce\x92" }, /* Β : up=Β : lo=β */
  { 0xce93 , 0xce93 , 0xceb3 , None , u8"\xce\x93" }, /* Γ : up=Γ : lo=γ */
  { 0xce94 , 0xce94 , 0xceb4 , None , u8"\xce\x94" }, /* Δ : up=Δ : lo=δ */
  { 0xce95 , 0xce95 , 0xceb5 , None , u8"\xce\x95" }, /* Ε : up=Ε : lo=ε */
  { 0xce96 , 0xce96 , 0xceb6 , None , u8"\xce\x96" }, /* Ζ : up=Ζ : lo=ζ */
  { 0xce97 , 0xce97 , 0xceb7 , None , u8"\xce\x97" }, /* Η : up=Η : lo=η */
  { 0xce98 , 0xce98 , 0xceb8 , None , u8"\xce\x98" }, /* Θ : up=Θ : lo=θ */
  { 0xce99 , 0xce99 , 0xceb9 , None , u8"\xce\x99" }, /* Ι : up=Ι : lo=ι */
  { 0xce9a , 0xce9a , 0xceba , None , u8"\xce\x9a" }, /* Κ : up=Κ : lo=κ */
  { 0xce9b , 0xce9b , 0xcebb , None , u8"\xce\x9b" }, /* Λ : up=Λ : lo=λ */
  { 0xce9c , 0xce9c , 0xcebc , None , u8"\xce\x9c" }, /* Μ : up=Μ : lo=μ */
  { 0xce9d , 0xce9d , 0xcebd , None , u8"\xce\x9d" }, /* Ν : up=Ν : lo=ν */
  { 0xce9e , 0xce9e , 0xcebe , None , u8"\xce\x9e" }, /* Ξ : up=Ξ : lo=ξ */
  { 0xce9f , 0xce9f , 0xcebf , None , u8"\xce\x9f" }, /* Ο : up=Ο : lo=ο */
  { 0xcea0 , 0xcea0 , 0xcf80 , None , u8"\xce\xa0" }, /* Π : up=Π : lo=π */
  { 0xcea1 , 0xcea1 , 0xcf81 , None , u8"\xce\xa1" }, /* Ρ : up=Ρ : lo=ρ */
  { 0xcea2 , 0xcea2 , 0xcea2 , None , u8"\xce\xa2" },
  { 0xcea3 , 0xcea3 , 0xcf83 , None , u8"\xce\xa3" }, /* Σ : up=Σ : lo=σ */
  { 0xcea4 , 0xcea4 , 0xcf84 , None , u8"\xce\xa4" }, /* Τ : up=Τ : lo=τ */
  { 0xcea5 , 0xcea5 , 0xcf85 , None , u8"\xce\xa5" }, /* Υ : up=Υ : lo=υ */
  { 0xcea6 , 0xcea6 , 0xcf86 , None , u8"\xce\xa6" }, /* Φ : up=Φ : lo=φ */
  { 0xcea7 , 0xcea7 , 0xcf87 , None , u8"\xce\xa7" }, /* Χ : up=Χ : lo=χ */
  { 0xcea8 , 0xcea8 , 0xcf88 , None , u8"\xce\xa8" }, /* Ψ : up=Ψ : lo=ψ */
  { 0xcea9 , 0xcea9 , 0xcf89 , None , u8"\xce\xa9" }, /* Ω : up=Ω : lo=ω */
  { 0xceaa , 0xceaa , 0xcf8a , None , u8"\xce\xaa" }, /* Ϊ : up=Ϊ : lo=ϊ */
  { 0xceab , 0xceab , 0xcf8b , None , u8"\xce\xab" }, /* Ϋ : up=Ϋ : lo=ϋ */
  { 0xceac , 0xce86 , 0xceac , None , u8"\xce\xac" }, /* ά : up=Ά : lo=ά */
  { 0xcead , 0xce88 , 0xcead , None , u8"\xce\xad" }, /* έ : up=Έ : lo=έ */
  { 0xceae , 0xce89 , 0xceae , None , u8"\xce\xae" }, /* ή : up=Ή : lo=ή */
  { 0xceaf , 0xce8a , 0xceaf , None , u8"\xce\xaf" }, /* ί : up=Ί : lo=ί */
  { 0xceb0 , 0xceb0 , 0xceb0 , None , u8"\xce\xb0" }, /* ΰ : up=ΰ : lo=ΰ */
  { 0xceb1 , 0xce91 , 0xceb1 , None , u8"\xce\xb1" }, /* α : up=Α : lo=α */
  { 0xceb2 , 0xce92 , 0xceb2 , None , u8"\xce\xb2" }, /* β : up=Β : lo=β */
  { 0xceb3 , 0xce93 , 0xceb3 , None , u8"\xce\xb3" }, /* γ : up=Γ : lo=γ */
  { 0xceb4 , 0xce94 , 0xceb4 , None , u8"\xce\xb4" }, /* δ : up=Δ : lo=δ */
  { 0xceb5 , 0xce95 , 0xceb5 , None , u8"\xce\xb5" }, /* ε : up=Ε : lo=ε */
  { 0xceb6 , 0xce96 , 0xceb6 , None , u8"\xce\xb6" }, /* ζ : up=Ζ : lo=ζ */
  { 0xceb7 , 0xce97 , 0xceb7 , None , u8"\xce\xb7" }, /* η : up=Η : lo=η */
  { 0xceb8 , 0xce98 , 0xceb8 , None , u8"\xce\xb8" }, /* θ : up=Θ : lo=θ */
  { 0xceb9 , 0xce99 , 0xceb9 , None , u8"\xce\xb9" }, /* ι : up=Ι : lo=ι */
  { 0xceba , 0xce9a , 0xceba , None , u8"\xce\xba" }, /* κ : up=Κ : lo=κ */
  { 0xcebb , 0xce9b , 0xcebb , None , u8"\xce\xbb" }, /* λ : up=Λ : lo=λ */
  { 0xcebc , 0xce9c , 0xcebc , None , u8"\xce\xbc" }, /* μ : up=Μ : lo=μ */
  { 0xcebd , 0xce9d , 0xcebd , None , u8"\xce\xbd" }, /* ν : up=Ν : lo=ν */
  { 0xcebe , 0xce9e , 0xcebe , None , u8"\xce\xbe" }, /* ξ : up=Ξ : lo=ξ */
  { 0xcebf , 0xce9f , 0xcebf , None , u8"\xce\xbf" }, /* ο : up=Ο : lo=ο */
};

const character charmap_cf[64] = {
  { 0xcf80 , 0xcea0 , 0xcf80 , None , u8"\xcf\x80" }, /* π : up=Π : lo=π */
  { 0xcf81 , 0xcea1 , 0xcf81 , None , u8"\xcf\x81" }, /* ρ : up=Ρ : lo=ρ */
  { 0xcf82 , 0xcea3 , 0xcf82 , None , u8"\xcf\x82" }, /* ς : up=Σ : lo=ς */
  { 0xcf83 , 0xcea3 , 0xcf83 , None , u8"\xcf\x83" }, /* σ : up=Σ : lo=σ */
  { 0xcf84 , 0xcea4 , 0xcf84 , None , u8"\xcf\x84" }, /* τ : up=Τ : lo=τ */
  { 0xcf85 , 0xcea5 , 0xcf85 , None , u8"\xcf\x85" }, /* υ : up=Υ : lo=υ */
  { 0xcf86 , 0xcea6 , 0xcf86 , None , u8"\xcf\x86" }, /* φ : up=Φ : lo=φ */
  { 0xcf87 , 0xcea7 , 0xcf87 , None , u8"\xcf\x87" }, /* χ : up=Χ : lo=χ */
  { 0xcf88 , 0xcea8 , 0xcf88 , None , u8"\xcf\x88" }, /* ψ : up=Ψ : lo=ψ */
  { 0xcf89 , 0xcea9 , 0xcf89 , None , u8"\xcf\x89" }, /* ω : up=Ω : lo=ω */
  { 0xcf8a , 0xceaa , 0xcf8a , None , u8"\xcf\x8a" }, /* ϊ : up=Ϊ : lo=ϊ */
  { 0xcf8b , 0xceab , 0xcf8b , None , u8"\xcf\x8b" }, /* ϋ : up=Ϋ : lo=ϋ */
  { 0xcf8c , 0xce8c , 0xcf8c , None , u8"\xcf\x8c" }, /* ό : up=Ό : lo=ό */
  { 0xcf8d , 0xce8e , 0xcf8d , None , u8"\xcf\x8d" }, /* ύ : up=Ύ : lo=ύ */
  { 0xcf8e , 0xce8f , 0xcf8e , None , u8"\xcf\x8e" }, /* ώ : up=Ώ : lo=ώ */
  { 0xcf8f , 0xcf8f , 0xcf97 , None , u8"\xcf\x8f" }, /* Ϗ : up=Ϗ : lo=ϗ */
  { 0xcf90 , 0xce92 , 0xcf90 , None , u8"\xcf\x90" }, /* ϐ : up=Β : lo=ϐ */
  { 0xcf91 , 0xce98 , 0xcf91 , None , u8"\xcf\x91" }, /* ϑ : up=Θ : lo=ϑ */
  { 0xcf92 , 0xcf92 , 0xcf92 , None , u8"\xcf\x92" }, /* ϒ : up=ϒ : lo=ϒ */
  { 0xcf93 , 0xcf93 , 0xcf93 , None , u8"\xcf\x93" }, /* ϓ : up=ϓ : lo=ϓ */
  { 0xcf94 , 0xcf94 , 0xcf94 , None , u8"\xcf\x94" }, /* ϔ : up=ϔ : lo=ϔ */
  { 0xcf95 , 0xcea6 , 0xcf95 , None , u8"\xcf\x95" }, /* ϕ : up=Φ : lo=ϕ */
  { 0xcf96 , 0xcea0 , 0xcf96 , None , u8"\xcf\x96" }, /* ϖ : up=Π : lo=ϖ */
  { 0xcf97 , 0xcf8f , 0xcf97 , None , u8"\xcf\x97" }, /* ϗ : up=Ϗ : lo=ϗ */
  { 0xcf98 , 0xcf98 , 0xcf99 , None , u8"\xcf\x98" }, /* Ϙ : up=Ϙ : lo=ϙ */
  { 0xcf99 , 0xcf98 , 0xcf99 , None , u8"\xcf\x99" }, /* ϙ : up=Ϙ : lo=ϙ */
  { 0xcf9a , 0xcf9a , 0xcf9b , None , u8"\xcf\x9a" }, /* Ϛ : up=Ϛ : lo=ϛ */
  { 0xcf9b , 0xcf9a , 0xcf9b , None , u8"\xcf\x9b" }, /* ϛ : up=Ϛ : lo=ϛ */
  { 0xcf9c , 0xcf9c , 0xcf9d , None , u8"\xcf\x9c" }, /* Ϝ : up=Ϝ : lo=ϝ */
  { 0xcf9d , 0xcf9c , 0xcf9d , None , u8"\xcf\x9d" }, /* ϝ : up=Ϝ : lo=ϝ */
  { 0xcf9e , 0xcf9e , 0xcf9f , None , u8"\xcf\x9e" }, /* Ϟ : up=Ϟ : lo=ϟ */
  { 0xcf9f , 0xcf9e , 0xcf9f , None , u8"\xcf\x9f" }, /* ϟ : up=Ϟ : lo=ϟ */
  { 0xcfa0 , 0xcfa0 , 0xcfa1 , None , u8"\xcf\xa0" }, /* Ϡ : up=Ϡ : lo=ϡ */
  { 0xcfa1 , 0xcfa0 , 0xcfa1 , None , u8"\xcf\xa1" }, /* ϡ : up=Ϡ : lo=ϡ */
  { 0xcfa2 , 0xcfa2 , 0xcfa3 , None , u8"\xcf\xa2" }, /* Ϣ : up=Ϣ : lo=ϣ */
  { 0xcfa3 , 0xcfa2 , 0xcfa3 , None , u8"\xcf\xa3" }, /* ϣ : up=Ϣ : lo=ϣ */
  { 0xcfa4 , 0xcfa4 , 0xcfa5 , None , u8"\xcf\xa4" }, /* Ϥ : up=Ϥ : lo=ϥ */
  { 0xcfa5 , 0xcfa4 , 0xcfa5 , None , u8"\xcf\xa5" }, /* ϥ : up=Ϥ : lo=ϥ */
  { 0xcfa6 , 0xcfa6 , 0xcfa7 , None , u8"\xcf\xa6" }, /* Ϧ : up=Ϧ : lo=ϧ */
  { 0xcfa7 , 0xcfa6 , 0xcfa7 , None , u8"\xcf\xa7" }, /* ϧ : up=Ϧ : lo=ϧ */
  { 0xcfa8 , 0xcfa8 , 0xcfa9 , None , u8"\xcf\xa8" }, /* Ϩ : up=Ϩ : lo=ϩ */
  { 0xcfa9 , 0xcfa8 , 0xcfa9 , None , u8"\xcf\xa9" }, /* ϩ : up=Ϩ : lo=ϩ */
  { 0xcfaa , 0xcfaa , 0xcfab , None , u8"\xcf\xaa" }, /* Ϫ : up=Ϫ : lo=ϫ */
  { 0xcfab , 0xcfaa , 0xcfab , None , u8"\xcf\xab" }, /* ϫ : up=Ϫ : lo=ϫ */
  { 0xcfac , 0xcfac , 0xcfad , None , u8"\xcf\xac" }, /* Ϭ : up=Ϭ : lo=ϭ */
  { 0xcfad , 0xcfac , 0xcfad , None , u8"\xcf\xad" }, /* ϭ : up=Ϭ : lo=ϭ */
  { 0xcfae , 0xcfae , 0xcfaf , None , u8"\xcf\xae" }, /* Ϯ : up=Ϯ : lo=ϯ */
  { 0xcfaf , 0xcfae , 0xcfaf , None , u8"\xcf\xaf" }, /* ϯ : up=Ϯ : lo=ϯ */
  { 0xcfb0 , 0xce9a , 0xcfb0 , None , u8"\xcf\xb0" }, /* ϰ : up=Κ : lo=ϰ */
  { 0xcfb1 , 0xcea1 , 0xcfb1 , None , u8"\xcf\xb1" }, /* ϱ : up=Ρ : lo=ϱ */
  { 0xcfb2 , 0xcfb9 , 0xcfb2 , None , u8"\xcf\xb2" }, /* ϲ : up=Ϲ : lo=ϲ */
  { 0xcfb3 , 0xcdbf , 0xcfb3 , None , u8"\xcf\xb3" }, /* ϳ : up=Ϳ : lo=ϳ */
  { 0xcfb4 , 0xcfb4 , 0xceb8 , None , u8"\xcf\xb4" }, /* ϴ : up=ϴ : lo=θ */
  { 0xcfb5 , 0xce95 , 0xcfb5 , None , u8"\xcf\xb5" }, /* ϵ : up=Ε : lo=ϵ */
  { 0xcfb6 , 0xcfb6 , 0xcfb6 , None , u8"\xcf\xb6" }, /* ϶ : up=϶ : lo=϶ */
  { 0xcfb7 , 0xcfb7 , 0xcfb8 , None , u8"\xcf\xb7" }, /* Ϸ : up=Ϸ : lo=ϸ */
  { 0xcfb8 , 0xcfb7 , 0xcfb8 , None , u8"\xcf\xb8" }, /* ϸ : up=Ϸ : lo=ϸ */
  { 0xcfb9 , 0xcfb9 , 0xcfb2 , None , u8"\xcf\xb9" }, /* Ϲ : up=Ϲ : lo=ϲ */
  { 0xcfba , 0xcfba , 0xcfbb , None , u8"\xcf\xba" }, /* Ϻ : up=Ϻ : lo=ϻ */
  { 0xcfbb , 0xcfba , 0xcfbb , None , u8"\xcf\xbb" }, /* ϻ : up=Ϻ : lo=ϻ */
  { 0xcfbc , 0xcfbc , 0xcfbc , None , u8"\xcf\xbc" }, /* ϼ : up=ϼ : lo=ϼ */
  { 0xcfbd , 0xcfbd , 0xcdbb , None , u8"\xcf\xbd" }, /* Ͻ : up=Ͻ : lo=ͻ */
  { 0xcfbe , 0xcfbe , 0xcdbc , None , u8"\xcf\xbe" }, /* Ͼ : up=Ͼ : lo=ͼ */
  { 0xcfbf , 0xcfbf , 0xcdbd , None , u8"\xcf\xbf" }, /* Ͽ : up=Ͽ : lo=ͽ */
};

const character charmap_d0[64] = {
  { 0xd080 , 0xd080 , 0xd190 , None , u8"\xd0\x80" }, /* Ѐ : up=Ѐ : lo=ѐ */
  { 0xd081 , 0xd081 , 0xd191 , None , u8"\xd0\x81" }, /* Ё : up=Ё : lo=ё */
  { 0xd082 , 0xd082 , 0xd192 , None , u8"\xd0\x82" }, /* Ђ : up=Ђ : lo=ђ */
  { 0xd083 , 0xd083 , 0xd193 , None , u8"\xd0\x83" }, /* Ѓ : up=Ѓ : lo=ѓ */
  { 0xd084 , 0xd084 , 0xd194 , None , u8"\xd0\x84" }, /* Є : up=Є : lo=є */
  { 0xd085 , 0xd085 , 0xd195 , None , u8"\xd0\x85" }, /* Ѕ : up=Ѕ : lo=ѕ */
  { 0xd086 , 0xd086 , 0xd196 , None , u8"\xd0\x86" }, /* І : up=І : lo=і */
  { 0xd087 , 0xd087 , 0xd197 , None , u8"\xd0\x87" }, /* Ї : up=Ї : lo=ї */
  { 0xd088 , 0xd088 , 0xd198 , None , u8"\xd0\x88" }, /* Ј : up=Ј : lo=ј */
  { 0xd089 , 0xd089 , 0xd199 , None , u8"\xd0\x89" }, /* Љ : up=Љ : lo=љ */
  { 0xd08a , 0xd08a , 0xd19a , None , u8"\xd0\x8a" }, /* Њ : up=Њ : lo=њ */
  { 0xd08b , 0xd08b , 0xd19b , None , u8"\xd0\x8b" }, /* Ћ : up=Ћ : lo=ћ */
  { 0xd08c , 0xd08c , 0xd19c , None , u8"\xd0\x8c" }, /* Ќ : up=Ќ : lo=ќ */
  { 0xd08d , 0xd08d , 0xd19d , None , u8"\xd0\x8d" }, /* Ѝ : up=Ѝ : lo=ѝ */
  { 0xd08e , 0xd08e , 0xd19e , None , u8"\xd0\x8e" }, /* Ў : up=Ў : lo=ў */
  { 0xd08f , 0xd08f , 0xd19f , None , u8"\xd0\x8f" }, /* Џ : up=Џ : lo=џ */
  { 0xd090 , 0xd090 , 0xd0b0 , None , u8"\xd0\x90" }, /* А : up=А : lo=а */
  { 0xd091 , 0xd091 , 0xd0b1 , None , u8"\xd0\x91" }, /* Б : up=Б : lo=б */
  { 0xd092 , 0xd092 , 0xd0b2 , None , u8"\xd0\x92" }, /* В : up=В : lo=в */
  { 0xd093 , 0xd093 , 0xd0b3 , None , u8"\xd0\x93" }, /* Г : up=Г : lo=г */
  { 0xd094 , 0xd094 , 0xd0b4 , None , u8"\xd0\x94" }, /* Д : up=Д : lo=д */
  { 0xd095 , 0xd095 , 0xd0b5 , None , u8"\xd0\x95" }, /* Е : up=Е : lo=е */
  { 0xd096 , 0xd096 , 0xd0b6 , None , u8"\xd0\x96" }, /* Ж : up=Ж : lo=ж */
  { 0xd097 , 0xd097 , 0xd0b7 , None , u8"\xd0\x97" }, /* З : up=З : lo=з */
  { 0xd098 , 0xd098 , 0xd0b8 , None , u8"\xd0\x98" }, /* И : up=И : lo=и */
  { 0xd099 , 0xd099 , 0xd0b9 , None , u8"\xd0\x99" }, /* Й : up=Й : lo=й */
  { 0xd09a , 0xd09a , 0xd0ba , None , u8"\xd0\x9a" }, /* К : up=К : lo=к */
  { 0xd09b , 0xd09b , 0xd0bb , None , u8"\xd0\x9b" }, /* Л : up=Л : lo=л */
  { 0xd09c , 0xd09c , 0xd0bc , None , u8"\xd0\x9c" }, /* М : up=М : lo=м */
  { 0xd09d , 0xd09d , 0xd0bd , None , u8"\xd0\x9d" }, /* Н : up=Н : lo=н */
  { 0xd09e , 0xd09e , 0xd0be , None , u8"\xd0\x9e" }, /* О : up=О : lo=о */
  { 0xd09f , 0xd09f , 0xd0bf , None , u8"\xd0\x9f" }, /* П : up=П : lo=п */
  { 0xd0a0 , 0xd0a0 , 0xd180 , None , u8"\xd0\xa0" }, /* Р : up=Р : lo=р */
  { 0xd0a1 , 0xd0a1 , 0xd181 , None , u8"\xd0\xa1" }, /* С : up=С : lo=с */
  { 0xd0a2 , 0xd0a2 , 0xd182 , None , u8"\xd0\xa2" }, /* Т : up=Т : lo=т */
  { 0xd0a3 , 0xd0a3 , 0xd183 , None , u8"\xd0\xa3" }, /* У : up=У : lo=у */
  { 0xd0a4 , 0xd0a4 , 0xd184 , None , u8"\xd0\xa4" }, /* Ф : up=Ф : lo=ф */
  { 0xd0a5 , 0xd0a5 , 0xd185 , None , u8"\xd0\xa5" }, /* Х : up=Х : lo=х */
  { 0xd0a6 , 0xd0a6 , 0xd186 , None , u8"\xd0\xa6" }, /* Ц : up=Ц : lo=ц */
  { 0xd0a7 , 0xd0a7 , 0xd187 , None , u8"\xd0\xa7" }, /* Ч : up=Ч : lo=ч */
  { 0xd0a8 , 0xd0a8 , 0xd188 , None , u8"\xd0\xa8" }, /* Ш : up=Ш : lo=ш */
  { 0xd0a9 , 0xd0a9 , 0xd189 , None , u8"\xd0\xa9" }, /* Щ : up=Щ : lo=щ */
  { 0xd0aa , 0xd0aa , 0xd18a , None , u8"\xd0\xaa" }, /* Ъ : up=Ъ : lo=ъ */
  { 0xd0ab , 0xd0ab , 0xd18b , None , u8"\xd0\xab" }, /* Ы : up=Ы : lo=ы */
  { 0xd0ac , 0xd0ac , 0xd18c , None , u8"\xd0\xac" }, /* Ь : up=Ь : lo=ь */
  { 0xd0ad , 0xd0ad , 0xd18d , None , u8"\xd0\xad" }, /* Э : up=Э : lo=э */
  { 0xd0ae , 0xd0ae , 0xd18e , None , u8"\xd0\xae" }, /* Ю : up=Ю : lo=ю */
  { 0xd0af , 0xd0af , 0xd18f , None , u8"\xd0\xaf" }, /* Я : up=Я : lo=я */
  { 0xd0b0 , 0xd090 , 0xd0b0 , None , u8"\xd0\xb0" }, /* а : up=А : lo=а */
  { 0xd0b1 , 0xd091 , 0xd0b1 , None , u8"\xd0\xb1" }, /* б : up=Б : lo=б */
  { 0xd0b2 , 0xd092 , 0xd0b2 , None , u8"\xd0\xb2" }, /* в : up=В : lo=в */
  { 0xd0b3 , 0xd093 , 0xd0b3 , None , u8"\xd0\xb3" }, /* г : up=Г : lo=г */
  { 0xd0b4 , 0xd094 , 0xd0b4 , None , u8"\xd0\xb4" }, /* д : up=Д : lo=д */
  { 0xd0b5 , 0xd095 , 0xd0b5 , None , u8"\xd0\xb5" }, /* е : up=Е : lo=е */
  { 0xd0b6 , 0xd096 , 0xd0b6 , None , u8"\xd0\xb6" }, /* ж : up=Ж : lo=ж */
  { 0xd0b7 , 0xd097 , 0xd0b7 , None , u8"\xd0\xb7" }, /* з : up=З : lo=з */
  { 0xd0b8 , 0xd098 , 0xd0b8 , None , u8"\xd0\xb8" }, /* и : up=И : lo=и */
  { 0xd0b9 , 0xd099 , 0xd0b9 , None , u8"\xd0\xb9" }, /* й : up=Й : lo=й */
  { 0xd0ba , 0xd09a , 0xd0ba , None , u8"\xd0\xba" }, /* к : up=К : lo=к */
  { 0xd0bb , 0xd09b , 0xd0bb , None , u8"\xd0\xbb" }, /* л : up=Л : lo=л */
  { 0xd0bc , 0xd09c , 0xd0bc , None , u8"\xd0\xbc" }, /* м : up=М : lo=м */
  { 0xd0bd , 0xd09d , 0xd0bd , None , u8"\xd0\xbd" }, /* н : up=Н : lo=н */
  { 0xd0be , 0xd09e , 0xd0be , None , u8"\xd0\xbe" }, /* о : up=О : lo=о */
  { 0xd0bf , 0xd09f , 0xd0bf , None , u8"\xd0\xbf" }, /* п : up=П : lo=п */
};

const character charmap_d1[64] = {
  { 0xd180 , 0xd0a0 , 0xd180 , None , u8"\xd1\x80" }, /* р : up=Р : lo=р */
  { 0xd181 , 0xd0a1 , 0xd181 , None , u8"\xd1\x81" }, /* с : up=С : lo=с */
  { 0xd182 , 0xd0a2 , 0xd182 , None , u8"\xd1\x82" }, /* т : up=Т : lo=т */
  { 0xd183 , 0xd0a3 , 0xd183 , None , u8"\xd1\x83" }, /* у : up=У : lo=у */
  { 0xd184 , 0xd0a4 , 0xd184 , None , u8"\xd1\x84" }, /* ф : up=Ф : lo=ф */
  { 0xd185 , 0xd0a5 , 0xd185 , None , u8"\xd1\x85" }, /* х : up=Х : lo=х */
  { 0xd186 , 0xd0a6 , 0xd186 , None , u8"\xd1\x86" }, /* ц : up=Ц : lo=ц */
  { 0xd187 , 0xd0a7 , 0xd187 , None , u8"\xd1\x87" }, /* ч : up=Ч : lo=ч */
  { 0xd188 , 0xd0a8 , 0xd188 , None , u8"\xd1\x88" }, /* ш : up=Ш : lo=ш */
  { 0xd189 , 0xd0a9 , 0xd189 , None , u8"\xd1\x89" }, /* щ : up=Щ : lo=щ */
  { 0xd18a , 0xd0aa , 0xd18a , None , u8"\xd1\x8a" }, /* ъ : up=Ъ : lo=ъ */
  { 0xd18b , 0xd0ab , 0xd18b , None , u8"\xd1\x8b" }, /* ы : up=Ы : lo=ы */
  { 0xd18c , 0xd0ac , 0xd18c , None , u8"\xd1\x8c" }, /* ь : up=Ь : lo=ь */
  { 0xd18d , 0xd0ad , 0xd18d , None , u8"\xd1\x8d" }, /* э : up=Э : lo=э */
  { 0xd18e , 0xd0ae , 0xd18e , None , u8"\xd1\x8e" }, /* ю : up=Ю : lo=ю */
  { 0xd18f , 0xd0af , 0xd18f , None , u8"\xd1\x8f" }, /* я : up=Я : lo=я */
  { 0xd190 , 0xd080 , 0xd190 , None , u8"\xd1\x90" }, /* ѐ : up=Ѐ : lo=ѐ */
  { 0xd191 , 0xd081 , 0xd191 , None , u8"\xd1\x91" }, /* ё : up=Ё : lo=ё */
  { 0xd192 , 0xd082 , 0xd192 , None , u8"\xd1\x92" }, /* ђ : up=Ђ : lo=ђ */
  { 0xd193 , 0xd083 , 0xd193 , None , u8"\xd1\x93" }, /* ѓ : up=Ѓ : lo=ѓ */
  { 0xd194 , 0xd084 , 0xd194 , None , u8"\xd1\x94" }, /* є : up=Є : lo=є */
  { 0xd195 , 0xd085 , 0xd195 , None , u8"\xd1\x95" }, /* ѕ : up=Ѕ : lo=ѕ */
  { 0xd196 , 0xd086 , 0xd196 , None , u8"\xd1\x96" }, /* і : up=І : lo=і */
  { 0xd197 , 0xd087 , 0xd197 , None , u8"\xd1\x97" }, /* ї : up=Ї : lo=ї */
  { 0xd198 , 0xd088 , 0xd198 , None , u8"\xd1\x98" }, /* ј : up=Ј : lo=ј */
  { 0xd199 , 0xd089 , 0xd199 , None , u8"\xd1\x99" }, /* љ : up=Љ : lo=љ */
  { 0xd19a , 0xd08a , 0xd19a , None , u8"\xd1\x9a" }, /* њ : up=Њ : lo=њ */
  { 0xd19b , 0xd08b , 0xd19b , None , u8"\xd1\x9b" }, /* ћ : up=Ћ : lo=ћ */
  { 0xd19c , 0xd08c , 0xd19c , None , u8"\xd1\x9c" }, /* ќ : up=Ќ : lo=ќ */
  { 0xd19d , 0xd08d , 0xd19d , None , u8"\xd1\x9d" }, /* ѝ : up=Ѝ : lo=ѝ */
  { 0xd19e , 0xd08e , 0xd19e , None , u8"\xd1\x9e" }, /* ў : up=Ў : lo=ў */
  { 0xd19f , 0xd08f , 0xd19f , None , u8"\xd1\x9f" }, /* џ : up=Џ : lo=џ */
  { 0xd1a0 , 0xd1a0 , 0xd1a1 , None , u8"\xd1\xa0" }, /* Ѡ : up=Ѡ : lo=ѡ */
  { 0xd1a1 , 0xd1a0 , 0xd1a1 , None , u8"\xd1\xa1" }, /* ѡ : up=Ѡ : lo=ѡ */
  { 0xd1a2 , 0xd1a2 , 0xd1a3 , None , u8"\xd1\xa2" }, /* Ѣ : up=Ѣ : lo=ѣ */
  { 0xd1a3 , 0xd1a2 , 0xd1a3 , None , u8"\xd1\xa3" }, /* ѣ : up=Ѣ : lo=ѣ */
  { 0xd1a4 , 0xd1a4 , 0xd1a5 , None , u8"\xd1\xa4" }, /* Ѥ : up=Ѥ : lo=ѥ */
  { 0xd1a5 , 0xd1a4 , 0xd1a5 , None , u8"\xd1\xa5" }, /* ѥ : up=Ѥ : lo=ѥ */
  { 0xd1a6 , 0xd1a6 , 0xd1a7 , None , u8"\xd1\xa6" }, /* Ѧ : up=Ѧ : lo=ѧ */
  { 0xd1a7 , 0xd1a6 , 0xd1a7 , None , u8"\xd1\xa7" }, /* ѧ : up=Ѧ : lo=ѧ */
  { 0xd1a8 , 0xd1a8 , 0xd1a9 , None , u8"\xd1\xa8" }, /* Ѩ : up=Ѩ : lo=ѩ */
  { 0xd1a9 , 0xd1a8 , 0xd1a9 , None , u8"\xd1\xa9" }, /* ѩ : up=Ѩ : lo=ѩ */
  { 0xd1aa , 0xd1aa , 0xd1ab , None , u8"\xd1\xaa" }, /* Ѫ : up=Ѫ : lo=ѫ */
  { 0xd1ab , 0xd1aa , 0xd1ab , None , u8"\xd1\xab" }, /* ѫ : up=Ѫ : lo=ѫ */
  { 0xd1ac , 0xd1ac , 0xd1ad , None , u8"\xd1\xac" }, /* Ѭ : up=Ѭ : lo=ѭ */
  { 0xd1ad , 0xd1ac , 0xd1ad , None , u8"\xd1\xad" }, /* ѭ : up=Ѭ : lo=ѭ */
  { 0xd1ae , 0xd1ae , 0xd1af , None , u8"\xd1\xae" }, /* Ѯ : up=Ѯ : lo=ѯ */
  { 0xd1af , 0xd1ae , 0xd1af , None , u8"\xd1\xaf" }, /* ѯ : up=Ѯ : lo=ѯ */
  { 0xd1b0 , 0xd1b0 , 0xd1b1 , None , u8"\xd1\xb0" }, /* Ѱ : up=Ѱ : lo=ѱ */
  { 0xd1b1 , 0xd1b0 , 0xd1b1 , None , u8"\xd1\xb1" }, /* ѱ : up=Ѱ : lo=ѱ */
  { 0xd1b2 , 0xd1b2 , 0xd1b3 , None , u8"\xd1\xb2" }, /* Ѳ : up=Ѳ : lo=ѳ */
  { 0xd1b3 , 0xd1b2 , 0xd1b3 , None , u8"\xd1\xb3" }, /* ѳ : up=Ѳ : lo=ѳ */
  { 0xd1b4 , 0xd1b4 , 0xd1b5 , None , u8"\xd1\xb4" }, /* Ѵ : up=Ѵ : lo=ѵ */
  { 0xd1b5 , 0xd1b4 , 0xd1b5 , None , u8"\xd1\xb5" }, /* ѵ : up=Ѵ : lo=ѵ */
  { 0xd1b6 , 0xd1b6 , 0xd1b7 , None , u8"\xd1\xb6" }, /* Ѷ : up=Ѷ : lo=ѷ */
  { 0xd1b7 , 0xd1b6 , 0xd1b7 , None , u8"\xd1\xb7" }, /* ѷ : up=Ѷ : lo=ѷ */
  { 0xd1b8 , 0xd1b8 , 0xd1b9 , None , u8"\xd1\xb8" }, /* Ѹ : up=Ѹ : lo=ѹ */
  { 0xd1b9 , 0xd1b8 , 0xd1b9 , None , u8"\xd1\xb9" }, /* ѹ : up=Ѹ : lo=ѹ */
  { 0xd1ba , 0xd1ba , 0xd1bb , None , u8"\xd1\xba" }, /* Ѻ : up=Ѻ : lo=ѻ */
  { 0xd1bb , 0xd1ba , 0xd1bb , None , u8"\xd1\xbb" }, /* ѻ : up=Ѻ : lo=ѻ */
  { 0xd1bc , 0xd1bc , 0xd1bd , None , u8"\xd1\xbc" }, /* Ѽ : up=Ѽ : lo=ѽ */
  { 0xd1bd , 0xd1bc , 0xd1bd , None , u8"\xd1\xbd" }, /* ѽ : up=Ѽ : lo=ѽ */
  { 0xd1be , 0xd1be , 0xd1bf , None , u8"\xd1\xbe" }, /* Ѿ : up=Ѿ : lo=ѿ */
  { 0xd1bf , 0xd1be , 0xd1bf , None , u8"\xd1\xbf" }, /* ѿ : up=Ѿ : lo=ѿ */
};

const character charmap_d2[64] = {
  { 0xd280 , 0xd280 , 0xd281 , None , u8"\xd2\x80" }, /* Ҁ : up=Ҁ : lo=ҁ */
  { 0xd281 , 0xd280 , 0xd281 , None , u8"\xd2\x81" }, /* ҁ : up=Ҁ : lo=ҁ */
  { 0xd282 , 0xd282 , 0xd282 , None , u8"\xd2\x82" }, /* ҂ : up=҂ : lo=҂ */
  { 0xd283 , 0xd283 , 0xd283 , IsDiacritic , u8"\xd2\x83" },
  { 0xd284 , 0xd284 , 0xd284 , IsDiacritic , u8"\xd2\x84" },
  { 0xd285 , 0xd285 , 0xd285 , IsDiacritic , u8"\xd2\x85" },
  { 0xd286 , 0xd286 , 0xd286 , IsDiacritic , u8"\xd2\x86" },
  { 0xd287 , 0xd287 , 0xd287 , IsDiacritic , u8"\xd2\x87" },
  { 0xd288 , 0xd288 , 0xd288 , IsDiacritic , u8"\xd2\x88" },
  { 0xd289 , 0xd289 , 0xd289 , IsDiacritic , u8"\xd2\x89" },
  { 0xd28a , 0xd28a , 0xd28b , None , u8"\xd2\x8a" }, /* Ҋ : up=Ҋ : lo=ҋ */
  { 0xd28b , 0xd28a , 0xd28b , None , u8"\xd2\x8b" }, /* ҋ : up=Ҋ : lo=ҋ */
  { 0xd28c , 0xd28c , 0xd28d , None , u8"\xd2\x8c" }, /* Ҍ : up=Ҍ : lo=ҍ */
  { 0xd28d , 0xd28c , 0xd28d , None , u8"\xd2\x8d" }, /* ҍ : up=Ҍ : lo=ҍ */
  { 0xd28e , 0xd28e , 0xd28f , None , u8"\xd2\x8e" }, /* Ҏ : up=Ҏ : lo=ҏ */
  { 0xd28f , 0xd28e , 0xd28f , None , u8"\xd2\x8f" }, /* ҏ : up=Ҏ : lo=ҏ */
  { 0xd290 , 0xd290 , 0xd291 , None , u8"\xd2\x90" }, /* Ґ : up=Ґ : lo=ґ */
  { 0xd291 , 0xd290 , 0xd291 , None , u8"\xd2\x91" }, /* ґ : up=Ґ : lo=ґ */
  { 0xd292 , 0xd292 , 0xd293 , None , u8"\xd2\x92" }, /* Ғ : up=Ғ : lo=ғ */
  { 0xd293 , 0xd292 , 0xd293 , None , u8"\xd2\x93" }, /* ғ : up=Ғ : lo=ғ */
  { 0xd294 , 0xd294 , 0xd295 , None , u8"\xd2\x94" }, /* Ҕ : up=Ҕ : lo=ҕ */
  { 0xd295 , 0xd294 , 0xd295 , None , u8"\xd2\x95" }, /* ҕ : up=Ҕ : lo=ҕ */
  { 0xd296 , 0xd296 , 0xd297 , None , u8"\xd2\x96" }, /* Җ : up=Җ : lo=җ */
  { 0xd297 , 0xd296 , 0xd297 , None , u8"\xd2\x97" }, /* җ : up=Җ : lo=җ */
  { 0xd298 , 0xd298 , 0xd299 , None , u8"\xd2\x98" }, /* Ҙ : up=Ҙ : lo=ҙ */
  { 0xd299 , 0xd298 , 0xd299 , None , u8"\xd2\x99" }, /* ҙ : up=Ҙ : lo=ҙ */
  { 0xd29a , 0xd29a , 0xd29b , None , u8"\xd2\x9a" }, /* Қ : up=Қ : lo=қ */
  { 0xd29b , 0xd29a , 0xd29b , None , u8"\xd2\x9b" }, /* қ : up=Қ : lo=қ */
  { 0xd29c , 0xd29c , 0xd29d , None , u8"\xd2\x9c" }, /* Ҝ : up=Ҝ : lo=ҝ */
  { 0xd29d , 0xd29c , 0xd29d , None , u8"\xd2\x9d" }, /* ҝ : up=Ҝ : lo=ҝ */
  { 0xd29e , 0xd29e , 0xd29f , None , u8"\xd2\x9e" }, /* Ҟ : up=Ҟ : lo=ҟ */
  { 0xd29f , 0xd29e , 0xd29f , None , u8"\xd2\x9f" }, /* ҟ : up=Ҟ : lo=ҟ */
  { 0xd2a0 , 0xd2a0 , 0xd2a1 , None , u8"\xd2\xa0" }, /* Ҡ : up=Ҡ : lo=ҡ */
  { 0xd2a1 , 0xd2a0 , 0xd2a1 , None , u8"\xd2\xa1" }, /* ҡ : up=Ҡ : lo=ҡ */
  { 0xd2a2 , 0xd2a2 , 0xd2a3 , None , u8"\xd2\xa2" }, /* Ң : up=Ң : lo=ң */
  { 0xd2a3 , 0xd2a2 , 0xd2a3 , None , u8"\xd2\xa3" }, /* ң : up=Ң : lo=ң */
  { 0xd2a4 , 0xd2a4 , 0xd2a5 , None , u8"\xd2\xa4" }, /* Ҥ : up=Ҥ : lo=ҥ */
  { 0xd2a5 , 0xd2a4 , 0xd2a5 , None , u8"\xd2\xa5" }, /* ҥ : up=Ҥ : lo=ҥ */
  { 0xd2a6 , 0xd2a6 , 0xd2a7 , None , u8"\xd2\xa6" }, /* Ҧ : up=Ҧ : lo=ҧ */
  { 0xd2a7 , 0xd2a6 , 0xd2a7 , None , u8"\xd2\xa7" }, /* ҧ : up=Ҧ : lo=ҧ */
  { 0xd2a8 , 0xd2a8 , 0xd2a9 , None , u8"\xd2\xa8" }, /* Ҩ : up=Ҩ : lo=ҩ */
  { 0xd2a9 , 0xd2a8 , 0xd2a9 , None , u8"\xd2\xa9" }, /* ҩ : up=Ҩ : lo=ҩ */
  { 0xd2aa , 0xd2aa , 0xd2ab , None , u8"\xd2\xaa" }, /* Ҫ : up=Ҫ : lo=ҫ */
  { 0xd2ab , 0xd2aa , 0xd2ab , None , u8"\xd2\xab" }, /* ҫ : up=Ҫ : lo=ҫ */
  { 0xd2ac , 0xd2ac , 0xd2ad , None , u8"\xd2\xac" }, /* Ҭ : up=Ҭ : lo=ҭ */
  { 0xd2ad , 0xd2ac , 0xd2ad , None , u8"\xd2\xad" }, /* ҭ : up=Ҭ : lo=ҭ */
  { 0xd2ae , 0xd2ae , 0xd2af , None , u8"\xd2\xae" }, /* Ү : up=Ү : lo=ү */
  { 0xd2af , 0xd2ae , 0xd2af , None , u8"\xd2\xaf" }, /* ү : up=Ү : lo=ү */
  { 0xd2b0 , 0xd2b0 , 0xd2b1 , None , u8"\xd2\xb0" }, /* Ұ : up=Ұ : lo=ұ */
  { 0xd2b1 , 0xd2b0 , 0xd2b1 , None , u8"\xd2\xb1" }, /* ұ : up=Ұ : lo=ұ */
  { 0xd2b2 , 0xd2b2 , 0xd2b3 , None , u8"\xd2\xb2" }, /* Ҳ : up=Ҳ : lo=ҳ */
  { 0xd2b3 , 0xd2b2 , 0xd2b3 , None , u8"\xd2\xb3" }, /* ҳ : up=Ҳ : lo=ҳ */
  { 0xd2b4 , 0xd2b4 , 0xd2b5 , None , u8"\xd2\xb4" }, /* Ҵ : up=Ҵ : lo=ҵ */
  { 0xd2b5 , 0xd2b4 , 0xd2b5 , None , u8"\xd2\xb5" }, /* ҵ : up=Ҵ : lo=ҵ */
  { 0xd2b6 , 0xd2b6 , 0xd2b7 , None , u8"\xd2\xb6" }, /* Ҷ : up=Ҷ : lo=ҷ */
  { 0xd2b7 , 0xd2b6 , 0xd2b7 , None , u8"\xd2\xb7" }, /* ҷ : up=Ҷ : lo=ҷ */
  { 0xd2b8 , 0xd2b8 , 0xd2b9 , None , u8"\xd2\xb8" }, /* Ҹ : up=Ҹ : lo=ҹ */
  { 0xd2b9 , 0xd2b8 , 0xd2b9 , None , u8"\xd2\xb9" }, /* ҹ : up=Ҹ : lo=ҹ */
  { 0xd2ba , 0xd2ba , 0xd2bb , None , u8"\xd2\xba" }, /* Һ : up=Һ : lo=һ */
  { 0xd2bb , 0xd2ba , 0xd2bb , None , u8"\xd2\xbb" }, /* һ : up=Һ : lo=һ */
  { 0xd2bc , 0xd2bc , 0xd2bd , None , u8"\xd2\xbc" }, /* Ҽ : up=Ҽ : lo=ҽ */
  { 0xd2bd , 0xd2bc , 0xd2bd , None , u8"\xd2\xbd" }, /* ҽ : up=Ҽ : lo=ҽ */
  { 0xd2be , 0xd2be , 0xd2bf , None , u8"\xd2\xbe" }, /* Ҿ : up=Ҿ : lo=ҿ */
  { 0xd2bf , 0xd2be , 0xd2bf , None , u8"\xd2\xbf" }, /* ҿ : up=Ҿ : lo=ҿ */
};

const character charmap_d3[64] = {
  { 0xd380 , 0xd380 , 0xd38f , None , u8"\xd3\x80" }, /* Ӏ : up=Ӏ : lo=ӏ */
  { 0xd381 , 0xd381 , 0xd382 , None , u8"\xd3\x81" }, /* Ӂ : up=Ӂ : lo=ӂ */
  { 0xd382 , 0xd381 , 0xd382 , None , u8"\xd3\x82" }, /* ӂ : up=Ӂ : lo=ӂ */
  { 0xd383 , 0xd383 , 0xd384 , None , u8"\xd3\x83" }, /* Ӄ : up=Ӄ : lo=ӄ */
  { 0xd384 , 0xd383 , 0xd384 , None , u8"\xd3\x84" }, /* ӄ : up=Ӄ : lo=ӄ */
  { 0xd385 , 0xd385 , 0xd386 , None , u8"\xd3\x85" }, /* Ӆ : up=Ӆ : lo=ӆ */
  { 0xd386 , 0xd385 , 0xd386 , None , u8"\xd3\x86" }, /* ӆ : up=Ӆ : lo=ӆ */
  { 0xd387 , 0xd387 , 0xd388 , None , u8"\xd3\x87" }, /* Ӈ : up=Ӈ : lo=ӈ */
  { 0xd388 , 0xd387 , 0xd388 , None , u8"\xd3\x88" }, /* ӈ : up=Ӈ : lo=ӈ */
  { 0xd389 , 0xd389 , 0xd38a , None , u8"\xd3\x89" }, /* Ӊ : up=Ӊ : lo=ӊ */
  { 0xd38a , 0xd389 , 0xd38a , None , u8"\xd3\x8a" }, /* ӊ : up=Ӊ : lo=ӊ */
  { 0xd38b , 0xd38b , 0xd38c , None , u8"\xd3\x8b" }, /* Ӌ : up=Ӌ : lo=ӌ */
  { 0xd38c , 0xd38b , 0xd38c , None , u8"\xd3\x8c" }, /* ӌ : up=Ӌ : lo=ӌ */
  { 0xd38d , 0xd38d , 0xd38e , None , u8"\xd3\x8d" }, /* Ӎ : up=Ӎ : lo=ӎ */
  { 0xd38e , 0xd38d , 0xd38e , None , u8"\xd3\x8e" }, /* ӎ : up=Ӎ : lo=ӎ */
  { 0xd38f , 0xd380 , 0xd38f , None , u8"\xd3\x8f" }, /* ӏ : up=Ӏ : lo=ӏ */
  { 0xd390 , 0xd390 , 0xd391 , None , u8"\xd3\x90" }, /* Ӑ : up=Ӑ : lo=ӑ */
  { 0xd391 , 0xd390 , 0xd391 , None , u8"\xd3\x91" }, /* ӑ : up=Ӑ : lo=ӑ */
  { 0xd392 , 0xd392 , 0xd393 , None , u8"\xd3\x92" }, /* Ӓ : up=Ӓ : lo=ӓ */
  { 0xd393 , 0xd392 , 0xd393 , None , u8"\xd3\x93" }, /* ӓ : up=Ӓ : lo=ӓ */
  { 0xd394 , 0xd394 , 0xd395 , None , u8"\xd3\x94" }, /* Ӕ : up=Ӕ : lo=ӕ */
  { 0xd395 , 0xd394 , 0xd395 , None , u8"\xd3\x95" }, /* ӕ : up=Ӕ : lo=ӕ */
  { 0xd396 , 0xd396 , 0xd397 , None , u8"\xd3\x96" }, /* Ӗ : up=Ӗ : lo=ӗ */
  { 0xd397 , 0xd396 , 0xd397 , None , u8"\xd3\x97" }, /* ӗ : up=Ӗ : lo=ӗ */
  { 0xd398 , 0xd398 , 0xd399 , None , u8"\xd3\x98" }, /* Ә : up=Ә : lo=ә */
  { 0xd399 , 0xd398 , 0xd399 , None , u8"\xd3\x99" }, /* ә : up=Ә : lo=ә */
  { 0xd39a , 0xd39a , 0xd39b , None , u8"\xd3\x9a" }, /* Ӛ : up=Ӛ : lo=ӛ */
  { 0xd39b , 0xd39a , 0xd39b , None , u8"\xd3\x9b" }, /* ӛ : up=Ӛ : lo=ӛ */
  { 0xd39c , 0xd39c , 0xd39d , None , u8"\xd3\x9c" }, /* Ӝ : up=Ӝ : lo=ӝ */
  { 0xd39d , 0xd39c , 0xd39d , None , u8"\xd3\x9d" }, /* ӝ : up=Ӝ : lo=ӝ */
  { 0xd39e , 0xd39e , 0xd39f , None , u8"\xd3\x9e" }, /* Ӟ : up=Ӟ : lo=ӟ */
  { 0xd39f , 0xd39e , 0xd39f , None , u8"\xd3\x9f" }, /* ӟ : up=Ӟ : lo=ӟ */
  { 0xd3a0 , 0xd3a0 , 0xd3a1 , None , u8"\xd3\xa0" }, /* Ӡ : up=Ӡ : lo=ӡ */
  { 0xd3a1 , 0xd3a0 , 0xd3a1 , None , u8"\xd3\xa1" }, /* ӡ : up=Ӡ : lo=ӡ */
  { 0xd3a2 , 0xd3a2 , 0xd3a3 , None , u8"\xd3\xa2" }, /* Ӣ : up=Ӣ : lo=ӣ */
  { 0xd3a3 , 0xd3a2 , 0xd3a3 , None , u8"\xd3\xa3" }, /* ӣ : up=Ӣ : lo=ӣ */
  { 0xd3a4 , 0xd3a4 , 0xd3a5 , None , u8"\xd3\xa4" }, /* Ӥ : up=Ӥ : lo=ӥ */
  { 0xd3a5 , 0xd3a4 , 0xd3a5 , None , u8"\xd3\xa5" }, /* ӥ : up=Ӥ : lo=ӥ */
  { 0xd3a6 , 0xd3a6 , 0xd3a7 , None , u8"\xd3\xa6" }, /* Ӧ : up=Ӧ : lo=ӧ */
  { 0xd3a7 , 0xd3a6 , 0xd3a7 , None , u8"\xd3\xa7" }, /* ӧ : up=Ӧ : lo=ӧ */
  { 0xd3a8 , 0xd3a8 , 0xd3a9 , None , u8"\xd3\xa8" }, /* Ө : up=Ө : lo=ө */
  { 0xd3a9 , 0xd3a8 , 0xd3a9 , None , u8"\xd3\xa9" }, /* ө : up=Ө : lo=ө */
  { 0xd3aa , 0xd3aa , 0xd3ab , None , u8"\xd3\xaa" }, /* Ӫ : up=Ӫ : lo=ӫ */
  { 0xd3ab , 0xd3aa , 0xd3ab , None , u8"\xd3\xab" }, /* ӫ : up=Ӫ : lo=ӫ */
  { 0xd3ac , 0xd3ac , 0xd3ad , None , u8"\xd3\xac" }, /* Ӭ : up=Ӭ : lo=ӭ */
  { 0xd3ad , 0xd3ac , 0xd3ad , None , u8"\xd3\xad" }, /* ӭ : up=Ӭ : lo=ӭ */
  { 0xd3ae , 0xd3ae , 0xd3af , None , u8"\xd3\xae" }, /* Ӯ : up=Ӯ : lo=ӯ */
  { 0xd3af , 0xd3ae , 0xd3af , None , u8"\xd3\xaf" }, /* ӯ : up=Ӯ : lo=ӯ */
  { 0xd3b0 , 0xd3b0 , 0xd3b1 , None , u8"\xd3\xb0" }, /* Ӱ : up=Ӱ : lo=ӱ */
  { 0xd3b1 , 0xd3b0 , 0xd3b1 , None , u8"\xd3\xb1" }, /* ӱ : up=Ӱ : lo=ӱ */
  { 0xd3b2 , 0xd3b2 , 0xd3b3 , None , u8"\xd3\xb2" }, /* Ӳ : up=Ӳ : lo=ӳ */
  { 0xd3b3 , 0xd3b2 , 0xd3b3 , None , u8"\xd3\xb3" }, /* ӳ : up=Ӳ : lo=ӳ */
  { 0xd3b4 , 0xd3b4 , 0xd3b5 , None , u8"\xd3\xb4" }, /* Ӵ : up=Ӵ : lo=ӵ */
  { 0xd3b5 , 0xd3b4 , 0xd3b5 , None , u8"\xd3\xb5" }, /* ӵ : up=Ӵ : lo=ӵ */
  { 0xd3b6 , 0xd3b6 , 0xd3b7 , None , u8"\xd3\xb6" }, /* Ӷ : up=Ӷ : lo=ӷ */
  { 0xd3b7 , 0xd3b6 , 0xd3b7 , None , u8"\xd3\xb7" }, /* ӷ : up=Ӷ : lo=ӷ */
  { 0xd3b8 , 0xd3b8 , 0xd3b9 , None , u8"\xd3\xb8" }, /* Ӹ : up=Ӹ : lo=ӹ */
  { 0xd3b9 , 0xd3b8 , 0xd3b9 , None , u8"\xd3\xb9" }, /* ӹ : up=Ӹ : lo=ӹ */
  { 0xd3ba , 0xd3ba , 0xd3bb , None , u8"\xd3\xba" }, /* Ӻ : up=Ӻ : lo=ӻ */
  { 0xd3bb , 0xd3ba , 0xd3bb , None , u8"\xd3\xbb" }, /* ӻ : up=Ӻ : lo=ӻ */
  { 0xd3bc , 0xd3bc , 0xd3bd , None , u8"\xd3\xbc" }, /* Ӽ : up=Ӽ : lo=ӽ */
  { 0xd3bd , 0xd3bc , 0xd3bd , None , u8"\xd3\xbd" }, /* ӽ : up=Ӽ : lo=ӽ */
  { 0xd3be , 0xd3be , 0xd3bf , None , u8"\xd3\xbe" }, /* Ӿ : up=Ӿ : lo=ӿ */
  { 0xd3bf , 0xd3be , 0xd3bf , None , u8"\xd3\xbf" }, /* ӿ : up=Ӿ : lo=ӿ */
};

const character charmap_d4[64] = {
  { 0xd480 , 0xd480 , 0xd481 , None , u8"\xd4\x80" }, /* Ԁ : up=Ԁ : lo=ԁ */
  { 0xd481 , 0xd480 , 0xd481 , None , u8"\xd4\x81" }, /* ԁ : up=Ԁ : lo=ԁ */
  { 0xd482 , 0xd482 , 0xd483 , None , u8"\xd4\x82" }, /* Ԃ : up=Ԃ : lo=ԃ */
  { 0xd483 , 0xd482 , 0xd483 , None , u8"\xd4\x83" }, /* ԃ : up=Ԃ : lo=ԃ */
  { 0xd484 , 0xd484 , 0xd485 , None , u8"\xd4\x84" }, /* Ԅ : up=Ԅ : lo=ԅ */
  { 0xd485 , 0xd484 , 0xd485 , None , u8"\xd4\x85" }, /* ԅ : up=Ԅ : lo=ԅ */
  { 0xd486 , 0xd486 , 0xd487 , None , u8"\xd4\x86" }, /* Ԇ : up=Ԇ : lo=ԇ */
  { 0xd487 , 0xd486 , 0xd487 , None , u8"\xd4\x87" }, /* ԇ : up=Ԇ : lo=ԇ */
  { 0xd488 , 0xd488 , 0xd489 , None , u8"\xd4\x88" }, /* Ԉ : up=Ԉ : lo=ԉ */
  { 0xd489 , 0xd488 , 0xd489 , None , u8"\xd4\x89" }, /* ԉ : up=Ԉ : lo=ԉ */
  { 0xd48a , 0xd48a , 0xd48b , None , u8"\xd4\x8a" }, /* Ԋ : up=Ԋ : lo=ԋ */
  { 0xd48b , 0xd48a , 0xd48b , None , u8"\xd4\x8b" }, /* ԋ : up=Ԋ : lo=ԋ */
  { 0xd48c , 0xd48c , 0xd48d , None , u8"\xd4\x8c" }, /* Ԍ : up=Ԍ : lo=ԍ */
  { 0xd48d , 0xd48c , 0xd48d , None , u8"\xd4\x8d" }, /* ԍ : up=Ԍ : lo=ԍ */
  { 0xd48e , 0xd48e , 0xd48f , None , u8"\xd4\x8e" }, /* Ԏ : up=Ԏ : lo=ԏ */
  { 0xd48f , 0xd48e , 0xd48f , None , u8"\xd4\x8f" }, /* ԏ : up=Ԏ : lo=ԏ */
  { 0xd490 , 0xd490 , 0xd491 , None , u8"\xd4\x90" }, /* Ԑ : up=Ԑ : lo=ԑ */
  { 0xd491 , 0xd490 , 0xd491 , None , u8"\xd4\x91" }, /* ԑ : up=Ԑ : lo=ԑ */
  { 0xd492 , 0xd492 , 0xd493 , None , u8"\xd4\x92" }, /* Ԓ : up=Ԓ : lo=ԓ */
  { 0xd493 , 0xd492 , 0xd493 , None , u8"\xd4\x93" }, /* ԓ : up=Ԓ : lo=ԓ */
  { 0xd494 , 0xd494 , 0xd495 , None , u8"\xd4\x94" }, /* Ԕ : up=Ԕ : lo=ԕ */
  { 0xd495 , 0xd494 , 0xd495 , None , u8"\xd4\x95" }, /* ԕ : up=Ԕ : lo=ԕ */
  { 0xd496 , 0xd496 , 0xd497 , None , u8"\xd4\x96" }, /* Ԗ : up=Ԗ : lo=ԗ */
  { 0xd497 , 0xd496 , 0xd497 , None , u8"\xd4\x97" }, /* ԗ : up=Ԗ : lo=ԗ */
  { 0xd498 , 0xd498 , 0xd499 , None , u8"\xd4\x98" }, /* Ԙ : up=Ԙ : lo=ԙ */
  { 0xd499 , 0xd498 , 0xd499 , None , u8"\xd4\x99" }, /* ԙ : up=Ԙ : lo=ԙ */
  { 0xd49a , 0xd49a , 0xd49b , None , u8"\xd4\x9a" }, /* Ԛ : up=Ԛ : lo=ԛ */
  { 0xd49b , 0xd49a , 0xd49b , None , u8"\xd4\x9b" }, /* ԛ : up=Ԛ : lo=ԛ */
  { 0xd49c , 0xd49c , 0xd49d , None , u8"\xd4\x9c" }, /* Ԝ : up=Ԝ : lo=ԝ */
  { 0xd49d , 0xd49c , 0xd49d , None , u8"\xd4\x9d" }, /* ԝ : up=Ԝ : lo=ԝ */
  { 0xd49e , 0xd49e , 0xd49f , None , u8"\xd4\x9e" }, /* Ԟ : up=Ԟ : lo=ԟ */
  { 0xd49f , 0xd49e , 0xd49f , None , u8"\xd4\x9f" }, /* ԟ : up=Ԟ : lo=ԟ */
  { 0xd4a0 , 0xd4a0 , 0xd4a1 , None , u8"\xd4\xa0" }, /* Ԡ : up=Ԡ : lo=ԡ */
  { 0xd4a1 , 0xd4a0 , 0xd4a1 , None , u8"\xd4\xa1" }, /* ԡ : up=Ԡ : lo=ԡ */
  { 0xd4a2 , 0xd4a2 , 0xd4a3 , None , u8"\xd4\xa2" }, /* Ԣ : up=Ԣ : lo=ԣ */
  { 0xd4a3 , 0xd4a2 , 0xd4a3 , None , u8"\xd4\xa3" }, /* ԣ : up=Ԣ : lo=ԣ */
  { 0xd4a4 , 0xd4a4 , 0xd4a5 , None , u8"\xd4\xa4" }, /* Ԥ : up=Ԥ : lo=ԥ */
  { 0xd4a5 , 0xd4a4 , 0xd4a5 , None , u8"\xd4\xa5" }, /* ԥ : up=Ԥ : lo=ԥ */
  { 0xd4a6 , 0xd4a6 , 0xd4a7 , None , u8"\xd4\xa6" }, /* Ԧ : up=Ԧ : lo=ԧ */
  { 0xd4a7 , 0xd4a6 , 0xd4a7 , None , u8"\xd4\xa7" }, /* ԧ : up=Ԧ : lo=ԧ */
  { 0xd4a8 , 0xd4a8 , 0xd4a9 , None , u8"\xd4\xa8" }, /* Ԩ : up=Ԩ : lo=ԩ */
  { 0xd4a9 , 0xd4a8 , 0xd4a9 , None , u8"\xd4\xa9" }, /* ԩ : up=Ԩ : lo=ԩ */
  { 0xd4aa , 0xd4aa , 0xd4ab , None , u8"\xd4\xaa" }, /* Ԫ : up=Ԫ : lo=ԫ */
  { 0xd4ab , 0xd4aa , 0xd4ab , None , u8"\xd4\xab" }, /* ԫ : up=Ԫ : lo=ԫ */
  { 0xd4ac , 0xd4ac , 0xd4ad , None , u8"\xd4\xac" }, /* Ԭ : up=Ԭ : lo=ԭ */
  { 0xd4ad , 0xd4ac , 0xd4ad , None , u8"\xd4\xad" }, /* ԭ : up=Ԭ : lo=ԭ */
  { 0xd4ae , 0xd4ae , 0xd4af , None , u8"\xd4\xae" }, /* Ԯ : up=Ԯ : lo=ԯ */
  { 0xd4af , 0xd4ae , 0xd4af , None , u8"\xd4\xaf" }, /* ԯ : up=Ԯ : lo=ԯ */
  { 0xd4b0 , 0xd4b0 , 0xd4b0 , None , u8"\xd4\xb0" }, /* ԰ : up=԰ : lo=԰ */
  { 0xd4b1 , 0xd4b1 , 0xd5a1 , None , u8"\xd4\xb1" }, /* Ա : up=Ա : lo=ա */
  { 0xd4b2 , 0xd4b2 , 0xd5a2 , None , u8"\xd4\xb2" }, /* Բ : up=Բ : lo=բ */
  { 0xd4b3 , 0xd4b3 , 0xd5a3 , None , u8"\xd4\xb3" }, /* Գ : up=Գ : lo=գ */
  { 0xd4b4 , 0xd4b4 , 0xd5a4 , None , u8"\xd4\xb4" }, /* Դ : up=Դ : lo=դ */
  { 0xd4b5 , 0xd4b5 , 0xd5a5 , None , u8"\xd4\xb5" }, /* Ե : up=Ե : lo=ե */
  { 0xd4b6 , 0xd4b6 , 0xd5a6 , None , u8"\xd4\xb6" }, /* Զ : up=Զ : lo=զ */
  { 0xd4b7 , 0xd4b7 , 0xd5a7 , None , u8"\xd4\xb7" }, /* Է : up=Է : lo=է */
  { 0xd4b8 , 0xd4b8 , 0xd5a8 , None , u8"\xd4\xb8" }, /* Ը : up=Ը : lo=ը */
  { 0xd4b9 , 0xd4b9 , 0xd5a9 , None , u8"\xd4\xb9" }, /* Թ : up=Թ : lo=թ */
  { 0xd4ba , 0xd4ba , 0xd5aa , None , u8"\xd4\xba" }, /* Ժ : up=Ժ : lo=ժ */
  { 0xd4bb , 0xd4bb , 0xd5ab , None , u8"\xd4\xbb" }, /* Ի : up=Ի : lo=ի */
  { 0xd4bc , 0xd4bc , 0xd5ac , None , u8"\xd4\xbc" }, /* Լ : up=Լ : lo=լ */
  { 0xd4bd , 0xd4bd , 0xd5ad , None , u8"\xd4\xbd" }, /* Խ : up=Խ : lo=խ */
  { 0xd4be , 0xd4be , 0xd5ae , None , u8"\xd4\xbe" }, /* Ծ : up=Ծ : lo=ծ */
  { 0xd4bf , 0xd4bf , 0xd5af , None , u8"\xd4\xbf" }, /* Կ : up=Կ : lo=կ */
};

const character charmap_d5[64] = {
  { 0xd580 , 0xd580 , 0xd5b0 , None , u8"\xd5\x80" }, /* Հ : up=Հ : lo=հ */
  { 0xd581 , 0xd581 , 0xd5b1 , None , u8"\xd5\x81" }, /* Ձ : up=Ձ : lo=ձ */
  { 0xd582 , 0xd582 , 0xd5b2 , None , u8"\xd5\x82" }, /* Ղ : up=Ղ : lo=ղ */
  { 0xd583 , 0xd583 , 0xd5b3 , None , u8"\xd5\x83" }, /* Ճ : up=Ճ : lo=ճ */
  { 0xd584 , 0xd584 , 0xd5b4 , None , u8"\xd5\x84" }, /* Մ : up=Մ : lo=մ */
  { 0xd585 , 0xd585 , 0xd5b5 , None , u8"\xd5\x85" }, /* Յ : up=Յ : lo=յ */
  { 0xd586 , 0xd586 , 0xd5b6 , None , u8"\xd5\x86" }, /* Ն : up=Ն : lo=ն */
  { 0xd587 , 0xd587 , 0xd5b7 , None , u8"\xd5\x87" }, /* Շ : up=Շ : lo=շ */
  { 0xd588 , 0xd588 , 0xd5b8 , None , u8"\xd5\x88" }, /* Ո : up=Ո : lo=ո */
  { 0xd589 , 0xd589 , 0xd5b9 , None , u8"\xd5\x89" }, /* Չ : up=Չ : lo=չ */
  { 0xd58a , 0xd58a , 0xd5ba , None , u8"\xd5\x8a" }, /* Պ : up=Պ : lo=պ */
  { 0xd58b , 0xd58b , 0xd5bb , None , u8"\xd5\x8b" }, /* Ջ : up=Ջ : lo=ջ */
  { 0xd58c , 0xd58c , 0xd5bc , None , u8"\xd5\x8c" }, /* Ռ : up=Ռ : lo=ռ */
  { 0xd58d , 0xd58d , 0xd5bd , None , u8"\xd5\x8d" }, /* Ս : up=Ս : lo=ս */
  { 0xd58e , 0xd58e , 0xd5be , None , u8"\xd5\x8e" }, /* Վ : up=Վ : lo=վ */
  { 0xd58f , 0xd58f , 0xd5bf , None , u8"\xd5\x8f" }, /* Տ : up=Տ : lo=տ */
  { 0xd590 , 0xd590 , 0xd680 , None , u8"\xd5\x90" }, /* Ր : up=Ր : lo=ր */
  { 0xd591 , 0xd591 , 0xd681 , None , u8"\xd5\x91" }, /* Ց : up=Ց : lo=ց */
  { 0xd592 , 0xd592 , 0xd682 , None , u8"\xd5\x92" }, /* Ւ : up=Ւ : lo=ւ */
  { 0xd593 , 0xd593 , 0xd683 , None , u8"\xd5\x93" }, /* Փ : up=Փ : lo=փ */
  { 0xd594 , 0xd594 , 0xd684 , None , u8"\xd5\x94" }, /* Ք : up=Ք : lo=ք */
  { 0xd595 , 0xd595 , 0xd685 , None , u8"\xd5\x95" }, /* Օ : up=Օ : lo=օ */
  { 0xd596 , 0xd596 , 0xd686 , None , u8"\xd5\x96" }, /* Ֆ : up=Ֆ : lo=ֆ */
  { 0xd597 , 0xd597 , 0xd597 , None , u8"\xd5\x97" },
  { 0xd598 , 0xd598 , 0xd598 , None , u8"\xd5\x98" },
  { 0xd599 , 0xd599 , 0xd599 , None , u8"\xd5\x99" }, /* ՙ : up=ՙ : lo=ՙ */
  { 0xd59a , 0xd59a , 0xd59a , None , u8"\xd5\x9a" }, /* ՚ : up=՚ : lo=՚ */
  { 0xd59b , 0xd59b , 0xd59b , None , u8"\xd5\x9b" }, /* ՛ : up=՛ : lo=՛ */
  { 0xd59c , 0xd59c , 0xd59c , None , u8"\xd5\x9c" }, /* ՜ : up=՜ : lo=՜ */
  { 0xd59d , 0xd59d , 0xd59d , None , u8"\xd5\x9d" }, /* ՝ : up=՝ : lo=՝ */
  { 0xd59e , 0xd59e , 0xd59e , None , u8"\xd5\x9e" }, /* ՞ : up=՞ : lo=՞ */
  { 0xd59f , 0xd59f , 0xd59f , None , u8"\xd5\x9f" }, /* ՟ : up=՟ : lo=՟ */
  { 0xd5a0 , 0xd5a0 , 0xd5a0 , None , u8"\xd5\xa0" },
  { 0xd5a1 , 0xd4b1 , 0xd5a1 , None , u8"\xd5\xa1" }, /* ա : up=Ա : lo=ա */
  { 0xd5a2 , 0xd4b2 , 0xd5a2 , None , u8"\xd5\xa2" }, /* բ : up=Բ : lo=բ */
  { 0xd5a3 , 0xd4b3 , 0xd5a3 , None , u8"\xd5\xa3" }, /* գ : up=Գ : lo=գ */
  { 0xd5a4 , 0xd4b4 , 0xd5a4 , None , u8"\xd5\xa4" }, /* դ : up=Դ : lo=դ */
  { 0xd5a5 , 0xd4b5 , 0xd5a5 , None , u8"\xd5\xa5" }, /* ե : up=Ե : lo=ե */
  { 0xd5a6 , 0xd4b6 , 0xd5a6 , None , u8"\xd5\xa6" }, /* զ : up=Զ : lo=զ */
  { 0xd5a7 , 0xd4b7 , 0xd5a7 , None , u8"\xd5\xa7" }, /* է : up=Է : lo=է */
  { 0xd5a8 , 0xd4b8 , 0xd5a8 , None , u8"\xd5\xa8" }, /* ը : up=Ը : lo=ը */
  { 0xd5a9 , 0xd4b9 , 0xd5a9 , None , u8"\xd5\xa9" }, /* թ : up=Թ : lo=թ */
  { 0xd5aa , 0xd4ba , 0xd5aa , None , u8"\xd5\xaa" }, /* ժ : up=Ժ : lo=ժ */
  { 0xd5ab , 0xd4bb , 0xd5ab , None , u8"\xd5\xab" }, /* ի : up=Ի : lo=ի */
  { 0xd5ac , 0xd4bc , 0xd5ac , None , u8"\xd5\xac" }, /* լ : up=Լ : lo=լ */
  { 0xd5ad , 0xd4bd , 0xd5ad , None , u8"\xd5\xad" }, /* խ : up=Խ : lo=խ */
  { 0xd5ae , 0xd4be , 0xd5ae , None , u8"\xd5\xae" }, /* ծ : up=Ծ : lo=ծ */
  { 0xd5af , 0xd4bf , 0xd5af , None , u8"\xd5\xaf" }, /* կ : up=Կ : lo=կ */
  { 0xd5b0 , 0xd580 , 0xd5b0 , None , u8"\xd5\xb0" }, /* հ : up=Հ : lo=հ */
  { 0xd5b1 , 0xd581 , 0xd5b1 , None , u8"\xd5\xb1" }, /* ձ : up=Ձ : lo=ձ */
  { 0xd5b2 , 0xd582 , 0xd5b2 , None , u8"\xd5\xb2" }, /* ղ : up=Ղ : lo=ղ */
  { 0xd5b3 , 0xd583 , 0xd5b3 , None , u8"\xd5\xb3" }, /* ճ : up=Ճ : lo=ճ */
  { 0xd5b4 , 0xd584 , 0xd5b4 , None , u8"\xd5\xb4" }, /* մ : up=Մ : lo=մ */
  { 0xd5b5 , 0xd585 , 0xd5b5 , None , u8"\xd5\xb5" }, /* յ : up=Յ : lo=յ */
  { 0xd5b6 , 0xd586 , 0xd5b6 , None , u8"\xd5\xb6" }, /* ն : up=Ն : lo=ն */
  { 0xd5b7 , 0xd587 , 0xd5b7 , None , u8"\xd5\xb7" }, /* շ : up=Շ : lo=շ */
  { 0xd5b8 , 0xd588 , 0xd5b8 , None , u8"\xd5\xb8" }, /* ո : up=Ո : lo=ո */
  { 0xd5b9 , 0xd589 , 0xd5b9 , None , u8"\xd5\xb9" }, /* չ : up=Չ : lo=չ */
  { 0xd5ba , 0xd58a , 0xd5ba , None , u8"\xd5\xba" }, /* պ : up=Պ : lo=պ */
  { 0xd5bb , 0xd58b , 0xd5bb , None , u8"\xd5\xbb" }, /* ջ : up=Ջ : lo=ջ */
  { 0xd5bc , 0xd58c , 0xd5bc , None , u8"\xd5\xbc" }, /* ռ : up=Ռ : lo=ռ */
  { 0xd5bd , 0xd58d , 0xd5bd , None , u8"\xd5\xbd" }, /* ս : up=Ս : lo=ս */
  { 0xd5be , 0xd58e , 0xd5be , None , u8"\xd5\xbe" }, /* վ : up=Վ : lo=վ */
  { 0xd5bf , 0xd58f , 0xd5bf , None , u8"\xd5\xbf" }, /* տ : up=Տ : lo=տ */
};

const character charmap_d6[64] = {
  { 0xd680 , 0xd590 , 0xd680 , None , u8"\xd6\x80" }, /* ր : up=Ր : lo=ր */
  { 0xd681 , 0xd591 , 0xd681 , None , u8"\xd6\x81" }, /* ց : up=Ց : lo=ց */
  { 0xd682 , 0xd592 , 0xd682 , None , u8"\xd6\x82" }, /* ւ : up=Ւ : lo=ւ */
  { 0xd683 , 0xd593 , 0xd683 , None , u8"\xd6\x83" }, /* փ : up=Փ : lo=փ */
  { 0xd684 , 0xd594 , 0xd684 , None , u8"\xd6\x84" }, /* ք : up=Ք : lo=ք */
  { 0xd685 , 0xd595 , 0xd685 , None , u8"\xd6\x85" }, /* օ : up=Օ : lo=օ */
  { 0xd686 , 0xd596 , 0xd686 , None , u8"\xd6\x86" }, /* ֆ : up=Ֆ : lo=ֆ */
  { 0xd687 , 0xd687 , 0xd687 , None , u8"\xd6\x87" }, /* և : up=և : lo=և */
  { 0xd688 , 0xd688 , 0xd688 , None , u8"\xd6\x88" },
  { 0xd689 , 0xd689 , 0xd689 , None , u8"\xd6\x89" },
  { 0xd68a , 0xd68a , 0xd68a , None , u8"\xd6\x8a" },
  { 0xd68b , 0xd68b , 0xd68b , None , u8"\xd6\x8b" },
  { 0xd68c , 0xd68c , 0xd68c , None , u8"\xd6\x8c" },
  { 0xd68d , 0xd68d , 0xd68d , None , u8"\xd6\x8d" },
  { 0xd68e , 0xd68e , 0xd68e , None , u8"\xd6\x8e" },
  { 0xd68f , 0xd68f , 0xd68f , None , u8"\xd6\x8f" },
  { 0xd690 , 0xd690 , 0xd690 , None , u8"\xd6\x90" },
  { 0xd691 , 0xd691 , 0xd691 , None , u8"\xd6\x91" },
  { 0xd692 , 0xd692 , 0xd692 , None , u8"\xd6\x92" },
  { 0xd693 , 0xd693 , 0xd693 , None , u8"\xd6\x93" },
  { 0xd694 , 0xd694 , 0xd694 , None , u8"\xd6\x94" },
  { 0xd695 , 0xd695 , 0xd695 , None , u8"\xd6\x95" },
  { 0xd696 , 0xd696 , 0xd696 , None , u8"\xd6\x96" },
  { 0xd697 , 0xd697 , 0xd697 , None , u8"\xd6\x97" },
  { 0xd698 , 0xd698 , 0xd698 , None , u8"\xd6\x98" },
  { 0xd699 , 0xd699 , 0xd699 , None , u8"\xd6\x99" },
  { 0xd69a , 0xd69a , 0xd69a , None , u8"\xd6\x9a" },
  { 0xd69b , 0xd69b , 0xd69b , None , u8"\xd6\x9b" },
  { 0xd69c , 0xd69c , 0xd69c , None , u8"\xd6\x9c" },
  { 0xd69d , 0xd69d , 0xd69d , None , u8"\xd6\x9d" },
  { 0xd69e , 0xd69e , 0xd69e , None , u8"\xd6\x9e" },
  { 0xd69f , 0xd69f , 0xd69f , None , u8"\xd6\x9f" },
  { 0xd6a0 , 0xd6a0 , 0xd6a0 , None , u8"\xd6\xa0" },
  { 0xd6a1 , 0xd6a1 , 0xd6a1 , None , u8"\xd6\xa1" },
  { 0xd6a2 , 0xd6a2 , 0xd6a2 , None , u8"\xd6\xa2" },
  { 0xd6a3 , 0xd6a3 , 0xd6a3 , None , u8"\xd6\xa3" },
  { 0xd6a4 , 0xd6a4 , 0xd6a4 , None , u8"\xd6\xa4" },
  { 0xd6a5 , 0xd6a5 , 0xd6a5 , None , u8"\xd6\xa5" },
  { 0xd6a6 , 0xd6a6 , 0xd6a6 , None , u8"\xd6\xa6" },
  { 0xd6a7 , 0xd6a7 , 0xd6a7 , None , u8"\xd6\xa7" },
  { 0xd6a8 , 0xd6a8 , 0xd6a8 , None , u8"\xd6\xa8" },
  { 0xd6a9 , 0xd6a9 , 0xd6a9 , None , u8"\xd6\xa9" },
  { 0xd6aa , 0xd6aa , 0xd6aa , None , u8"\xd6\xaa" },
  { 0xd6ab , 0xd6ab , 0xd6ab , None , u8"\xd6\xab" },
  { 0xd6ac , 0xd6ac , 0xd6ac , None , u8"\xd6\xac" },
  { 0xd6ad , 0xd6ad , 0xd6ad , None , u8"\xd6\xad" },
  { 0xd6ae , 0xd6ae , 0xd6ae , None , u8"\xd6\xae" },
  { 0xd6af , 0xd6af , 0xd6af , None , u8"\xd6\xaf" },
  { 0xd6b0 , 0xd6b0 , 0xd6b0 , None , u8"\xd6\xb0" },
  { 0xd6b1 , 0xd6b1 , 0xd6b1 , None , u8"\xd6\xb1" },
  { 0xd6b2 , 0xd6b2 , 0xd6b2 , None , u8"\xd6\xb2" },
  { 0xd6b3 , 0xd6b3 , 0xd6b3 , None , u8"\xd6\xb3" },
  { 0xd6b4 , 0xd6b4 , 0xd6b4 , None , u8"\xd6\xb4" },
  { 0xd6b5 , 0xd6b5 , 0xd6b5 , None , u8"\xd6\xb5" },
  { 0xd6b6 , 0xd6b6 , 0xd6b6 , None , u8"\xd6\xb6" },
  { 0xd6b7 , 0xd6b7 , 0xd6b7 , None , u8"\xd6\xb7" },
  { 0xd6b8 , 0xd6b8 , 0xd6b8 , None , u8"\xd6\xb8" },
  { 0xd6b9 , 0xd6b9 , 0xd6b9 , None , u8"\xd6\xb9" },
  { 0xd6ba , 0xd6ba , 0xd6ba , None , u8"\xd6\xba" },
  { 0xd6bb , 0xd6bb , 0xd6bb , None , u8"\xd6\xbb" },
  { 0xd6bc , 0xd6bc , 0xd6bc , None , u8"\xd6\xbc" },
  { 0xd6bd , 0xd6bd , 0xd6bd , None , u8"\xd6\xbd" },
  { 0xd6be , 0xd6be , 0xd6be , None , u8"\xd6\xbe" },
  { 0xd6bf , 0xd6bf , 0xd6bf , None , u8"\xd6\xbf" },
};

const character* pagemap_24_e1[64] = {
  /* 80 */
  nullptr, nullptr, charmap_e1_82, charmap_e1_83, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  charmap_e1_b8, charmap_e1_b9, charmap_e1_ba, charmap_e1_bb, charmap_e1_bc, charmap_e1_bd, charmap_e1_be, charmap_e1_bf,
};

const character charmap_e1_82[64] = {
  { 0xe18280 , 0xe18280 , 0xe18280 , None , u8"\xe1\x82\x80" }, /* ႀ : up=ႀ : lo=ႀ */
  { 0xe18281 , 0xe18281 , 0xe18281 , None , u8"\xe1\x82\x81" }, /* ႁ : up=ႁ : lo=ႁ */
  { 0xe18282 , 0xe18282 , 0xe18282 , None , u8"\xe1\x82\x82" },
  { 0xe18283 , 0xe18283 , 0xe18283 , None , u8"\xe1\x82\x83" },
  { 0xe18284 , 0xe18284 , 0xe18284 , None , u8"\xe1\x82\x84" },
  { 0xe18285 , 0xe18285 , 0xe18285 , None , u8"\xe1\x82\x85" },
  { 0xe18286 , 0xe18286 , 0xe18286 , None , u8"\xe1\x82\x86" },
  { 0xe18287 , 0xe18287 , 0xe18287 , None , u8"\xe1\x82\x87" },
  { 0xe18288 , 0xe18288 , 0xe18288 , None , u8"\xe1\x82\x88" },
  { 0xe18289 , 0xe18289 , 0xe18289 , None , u8"\xe1\x82\x89" },
  { 0xe1828a , 0xe1828a , 0xe1828a , None , u8"\xe1\x82\x8a" },
  { 0xe1828b , 0xe1828b , 0xe1828b , None , u8"\xe1\x82\x8b" },
  { 0xe1828c , 0xe1828c , 0xe1828c , None , u8"\xe1\x82\x8c" },
  { 0xe1828d , 0xe1828d , 0xe1828d , None , u8"\xe1\x82\x8d" },
  { 0xe1828e , 0xe1828e , 0xe1828e , None , u8"\xe1\x82\x8e" }, /* ႎ : up=ႎ : lo=ႎ */
  { 0xe1828f , 0xe1828f , 0xe1828f , None , u8"\xe1\x82\x8f" },
  { 0xe18290 , 0xe18290 , 0xe18290 , None , u8"\xe1\x82\x90" }, /* ႐ : up=႐ : lo=႐ */
  { 0xe18291 , 0xe18291 , 0xe18291 , None , u8"\xe1\x82\x91" }, /* ႑ : up=႑ : lo=႑ */
  { 0xe18292 , 0xe18292 , 0xe18292 , None , u8"\xe1\x82\x92" }, /* ႒ : up=႒ : lo=႒ */
  { 0xe18293 , 0xe18293 , 0xe18293 , None , u8"\xe1\x82\x93" }, /* ႓ : up=႓ : lo=႓ */
  { 0xe18294 , 0xe18294 , 0xe18294 , None , u8"\xe1\x82\x94" }, /* ႔ : up=႔ : lo=႔ */
  { 0xe18295 , 0xe18295 , 0xe18295 , None , u8"\xe1\x82\x95" }, /* ႕ : up=႕ : lo=႕ */
  { 0xe18296 , 0xe18296 , 0xe18296 , None , u8"\xe1\x82\x96" }, /* ႖ : up=႖ : lo=႖ */
  { 0xe18297 , 0xe18297 , 0xe18297 , None , u8"\xe1\x82\x97" }, /* ႗ : up=႗ : lo=႗ */
  { 0xe18298 , 0xe18298 , 0xe18298 , None , u8"\xe1\x82\x98" }, /* ႘ : up=႘ : lo=႘ */
  { 0xe18299 , 0xe18299 , 0xe18299 , None , u8"\xe1\x82\x99" }, /* ႙ : up=႙ : lo=႙ */
  { 0xe1829a , 0xe1829a , 0xe1829a , None , u8"\xe1\x82\x9a" },
  { 0xe1829b , 0xe1829b , 0xe1829b , None , u8"\xe1\x82\x9b" },
  { 0xe1829c , 0xe1829c , 0xe1829c , None , u8"\xe1\x82\x9c" },
  { 0xe1829d , 0xe1829d , 0xe1829d , None , u8"\xe1\x82\x9d" },
  { 0xe1829e , 0xe1829e , 0xe1829e , None , u8"\xe1\x82\x9e" }, /* ႞ : up=႞ : lo=႞ */
  { 0xe1829f , 0xe1829f , 0xe1829f , None , u8"\xe1\x82\x9f" }, /* ႟ : up=႟ : lo=႟ */
  { 0xe182a0 , 0xe182a0 , 0xe2b480 , None , u8"\xe1\x82\xa0" }, /* Ⴀ : up=Ⴀ : lo=ⴀ */
  { 0xe182a1 , 0xe182a1 , 0xe2b481 , None , u8"\xe1\x82\xa1" }, /* Ⴁ : up=Ⴁ : lo=ⴁ */
  { 0xe182a2 , 0xe182a2 , 0xe2b482 , None , u8"\xe1\x82\xa2" }, /* Ⴂ : up=Ⴂ : lo=ⴂ */
  { 0xe182a3 , 0xe182a3 , 0xe2b483 , None , u8"\xe1\x82\xa3" }, /* Ⴃ : up=Ⴃ : lo=ⴃ */
  { 0xe182a4 , 0xe182a4 , 0xe2b484 , None , u8"\xe1\x82\xa4" }, /* Ⴄ : up=Ⴄ : lo=ⴄ */
  { 0xe182a5 , 0xe182a5 , 0xe2b485 , None , u8"\xe1\x82\xa5" }, /* Ⴅ : up=Ⴅ : lo=ⴅ */
  { 0xe182a6 , 0xe182a6 , 0xe2b486 , None , u8"\xe1\x82\xa6" }, /* Ⴆ : up=Ⴆ : lo=ⴆ */
  { 0xe182a7 , 0xe182a7 , 0xe2b487 , None , u8"\xe1\x82\xa7" }, /* Ⴇ : up=Ⴇ : lo=ⴇ */
  { 0xe182a8 , 0xe182a8 , 0xe2b488 , None , u8"\xe1\x82\xa8" }, /* Ⴈ : up=Ⴈ : lo=ⴈ */
  { 0xe182a9 , 0xe182a9 , 0xe2b489 , None , u8"\xe1\x82\xa9" }, /* Ⴉ : up=Ⴉ : lo=ⴉ */
  { 0xe182aa , 0xe182aa , 0xe2b48a , None , u8"\xe1\x82\xaa" }, /* Ⴊ : up=Ⴊ : lo=ⴊ */
  { 0xe182ab , 0xe182ab , 0xe2b48b , None , u8"\xe1\x82\xab" }, /* Ⴋ : up=Ⴋ : lo=ⴋ */
  { 0xe182ac , 0xe182ac , 0xe2b48c , None , u8"\xe1\x82\xac" }, /* Ⴌ : up=Ⴌ : lo=ⴌ */
  { 0xe182ad , 0xe182ad , 0xe2b48d , None , u8"\xe1\x82\xad" }, /* Ⴍ : up=Ⴍ : lo=ⴍ */
  { 0xe182ae , 0xe182ae , 0xe2b48e , None , u8"\xe1\x82\xae" }, /* Ⴎ : up=Ⴎ : lo=ⴎ */
  { 0xe182af , 0xe182af , 0xe2b48f , None , u8"\xe1\x82\xaf" }, /* Ⴏ : up=Ⴏ : lo=ⴏ */
  { 0xe182b0 , 0xe182b0 , 0xe2b490 , None , u8"\xe1\x82\xb0" }, /* Ⴐ : up=Ⴐ : lo=ⴐ */
  { 0xe182b1 , 0xe182b1 , 0xe2b491 , None , u8"\xe1\x82\xb1" }, /* Ⴑ : up=Ⴑ : lo=ⴑ */
  { 0xe182b2 , 0xe182b2 , 0xe2b492 , None , u8"\xe1\x82\xb2" }, /* Ⴒ : up=Ⴒ : lo=ⴒ */
  { 0xe182b3 , 0xe182b3 , 0xe2b493 , None , u8"\xe1\x82\xb3" }, /* Ⴓ : up=Ⴓ : lo=ⴓ */
  { 0xe182b4 , 0xe182b4 , 0xe2b494 , None , u8"\xe1\x82\xb4" }, /* Ⴔ : up=Ⴔ : lo=ⴔ */
  { 0xe182b5 , 0xe182b5 , 0xe2b495 , None , u8"\xe1\x82\xb5" }, /* Ⴕ : up=Ⴕ : lo=ⴕ */
  { 0xe182b6 , 0xe182b6 , 0xe2b496 , None , u8"\xe1\x82\xb6" }, /* Ⴖ : up=Ⴖ : lo=ⴖ */
  { 0xe182b7 , 0xe182b7 , 0xe2b497 , None , u8"\xe1\x82\xb7" }, /* Ⴗ : up=Ⴗ : lo=ⴗ */
  { 0xe182b8 , 0xe182b8 , 0xe2b498 , None , u8"\xe1\x82\xb8" }, /* Ⴘ : up=Ⴘ : lo=ⴘ */
  { 0xe182b9 , 0xe182b9 , 0xe2b499 , None , u8"\xe1\x82\xb9" }, /* Ⴙ : up=Ⴙ : lo=ⴙ */
  { 0xe182ba , 0xe182ba , 0xe2b49a , None , u8"\xe1\x82\xba" }, /* Ⴚ : up=Ⴚ : lo=ⴚ */
  { 0xe182bb , 0xe182bb , 0xe2b49b , None , u8"\xe1\x82\xbb" }, /* Ⴛ : up=Ⴛ : lo=ⴛ */
  { 0xe182bc , 0xe182bc , 0xe2b49c , None , u8"\xe1\x82\xbc" }, /* Ⴜ : up=Ⴜ : lo=ⴜ */
  { 0xe182bd , 0xe182bd , 0xe2b49d , None , u8"\xe1\x82\xbd" }, /* Ⴝ : up=Ⴝ : lo=ⴝ */
  { 0xe182be , 0xe182be , 0xe2b49e , None , u8"\xe1\x82\xbe" }, /* Ⴞ : up=Ⴞ : lo=ⴞ */
  { 0xe182bf , 0xe182bf , 0xe2b49f , None , u8"\xe1\x82\xbf" }, /* Ⴟ : up=Ⴟ : lo=ⴟ */
};

const character charmap_e1_83[64] = {
  { 0xe18380 , 0xe18380 , 0xe2b4a0 , None , u8"\xe1\x83\x80" }, /* Ⴠ : up=Ⴠ : lo=ⴠ */
  { 0xe18381 , 0xe18381 , 0xe2b4a1 , None , u8"\xe1\x83\x81" }, /* Ⴡ : up=Ⴡ : lo=ⴡ */
  { 0xe18382 , 0xe18382 , 0xe2b4a2 , None , u8"\xe1\x83\x82" }, /* Ⴢ : up=Ⴢ : lo=ⴢ */
  { 0xe18383 , 0xe18383 , 0xe2b4a3 , None , u8"\xe1\x83\x83" }, /* Ⴣ : up=Ⴣ : lo=ⴣ */
  { 0xe18384 , 0xe18384 , 0xe2b4a4 , None , u8"\xe1\x83\x84" }, /* Ⴤ : up=Ⴤ : lo=ⴤ */
  { 0xe18385 , 0xe18385 , 0xe2b4a5 , None , u8"\xe1\x83\x85" }, /* Ⴥ : up=Ⴥ : lo=ⴥ */
  { 0xe18386 , 0xe18386 , 0xe18386 , None , u8"\xe1\x83\x86" },
  { 0xe18387 , 0xe18387 , 0xe2b4a7 , None , u8"\xe1\x83\x87" }, /* Ⴧ : up=Ⴧ : lo=ⴧ */
  { 0xe18388 , 0xe18388 , 0xe18388 , None , u8"\xe1\x83\x88" },
  { 0xe18389 , 0xe18389 , 0xe18389 , None , u8"\xe1\x83\x89" },
  { 0xe1838a , 0xe1838a , 0xe1838a , None , u8"\xe1\x83\x8a" },
  { 0xe1838b , 0xe1838b , 0xe1838b , None , u8"\xe1\x83\x8b" },
  { 0xe1838c , 0xe1838c , 0xe1838c , None , u8"\xe1\x83\x8c" },
  { 0xe1838d , 0xe1838d , 0xe2b4ad , None , u8"\xe1\x83\x8d" }, /* Ⴭ : up=Ⴭ : lo=ⴭ */
  { 0xe1838e , 0xe1838e , 0xe1838e , None , u8"\xe1\x83\x8e" },
  { 0xe1838f , 0xe1838f , 0xe1838f , None , u8"\xe1\x83\x8f" },
  { 0xe18390 , 0xe18390 , 0xe18390 , None , u8"\xe1\x83\x90" }, /* ა : up=ა : lo=ა */
  { 0xe18391 , 0xe18391 , 0xe18391 , None , u8"\xe1\x83\x91" }, /* ბ : up=ბ : lo=ბ */
  { 0xe18392 , 0xe18392 , 0xe18392 , None , u8"\xe1\x83\x92" }, /* გ : up=გ : lo=გ */
  { 0xe18393 , 0xe18393 , 0xe18393 , None , u8"\xe1\x83\x93" }, /* დ : up=დ : lo=დ */
  { 0xe18394 , 0xe18394 , 0xe18394 , None , u8"\xe1\x83\x94" }, /* ე : up=ე : lo=ე */
  { 0xe18395 , 0xe18395 , 0xe18395 , None , u8"\xe1\x83\x95" }, /* ვ : up=ვ : lo=ვ */
  { 0xe18396 , 0xe18396 , 0xe18396 , None , u8"\xe1\x83\x96" }, /* ზ : up=ზ : lo=ზ */
  { 0xe18397 , 0xe18397 , 0xe18397 , None , u8"\xe1\x83\x97" }, /* თ : up=თ : lo=თ */
  { 0xe18398 , 0xe18398 , 0xe18398 , None , u8"\xe1\x83\x98" }, /* ი : up=ი : lo=ი */
  { 0xe18399 , 0xe18399 , 0xe18399 , None , u8"\xe1\x83\x99" }, /* კ : up=კ : lo=კ */
  { 0xe1839a , 0xe1839a , 0xe1839a , None , u8"\xe1\x83\x9a" }, /* ლ : up=ლ : lo=ლ */
  { 0xe1839b , 0xe1839b , 0xe1839b , None , u8"\xe1\x83\x9b" }, /* მ : up=მ : lo=მ */
  { 0xe1839c , 0xe1839c , 0xe1839c , None , u8"\xe1\x83\x9c" }, /* ნ : up=ნ : lo=ნ */
  { 0xe1839d , 0xe1839d , 0xe1839d , None , u8"\xe1\x83\x9d" }, /* ო : up=ო : lo=ო */
  { 0xe1839e , 0xe1839e , 0xe1839e , None , u8"\xe1\x83\x9e" }, /* პ : up=პ : lo=პ */
  { 0xe1839f , 0xe1839f , 0xe1839f , None , u8"\xe1\x83\x9f" }, /* ჟ : up=ჟ : lo=ჟ */
  { 0xe183a0 , 0xe183a0 , 0xe183a0 , None , u8"\xe1\x83\xa0" }, /* რ : up=რ : lo=რ */
  { 0xe183a1 , 0xe183a1 , 0xe183a1 , None , u8"\xe1\x83\xa1" }, /* ს : up=ს : lo=ს */
  { 0xe183a2 , 0xe183a2 , 0xe183a2 , None , u8"\xe1\x83\xa2" }, /* ტ : up=ტ : lo=ტ */
  { 0xe183a3 , 0xe183a3 , 0xe183a3 , None , u8"\xe1\x83\xa3" }, /* უ : up=უ : lo=უ */
  { 0xe183a4 , 0xe183a4 , 0xe183a4 , None , u8"\xe1\x83\xa4" }, /* ფ : up=ფ : lo=ფ */
  { 0xe183a5 , 0xe183a5 , 0xe183a5 , None , u8"\xe1\x83\xa5" }, /* ქ : up=ქ : lo=ქ */
  { 0xe183a6 , 0xe183a6 , 0xe183a6 , None , u8"\xe1\x83\xa6" }, /* ღ : up=ღ : lo=ღ */
  { 0xe183a7 , 0xe183a7 , 0xe183a7 , None , u8"\xe1\x83\xa7" }, /* ყ : up=ყ : lo=ყ */
  { 0xe183a8 , 0xe183a8 , 0xe183a8 , None , u8"\xe1\x83\xa8" }, /* შ : up=შ : lo=შ */
  { 0xe183a9 , 0xe183a9 , 0xe183a9 , None , u8"\xe1\x83\xa9" }, /* ჩ : up=ჩ : lo=ჩ */
  { 0xe183aa , 0xe183aa , 0xe183aa , None , u8"\xe1\x83\xaa" }, /* ც : up=ც : lo=ც */
  { 0xe183ab , 0xe183ab , 0xe183ab , None , u8"\xe1\x83\xab" }, /* ძ : up=ძ : lo=ძ */
  { 0xe183ac , 0xe183ac , 0xe183ac , None , u8"\xe1\x83\xac" }, /* წ : up=წ : lo=წ */
  { 0xe183ad , 0xe183ad , 0xe183ad , None , u8"\xe1\x83\xad" }, /* ჭ : up=ჭ : lo=ჭ */
  { 0xe183ae , 0xe183ae , 0xe183ae , None , u8"\xe1\x83\xae" }, /* ხ : up=ხ : lo=ხ */
  { 0xe183af , 0xe183af , 0xe183af , None , u8"\xe1\x83\xaf" }, /* ჯ : up=ჯ : lo=ჯ */
  { 0xe183b0 , 0xe183b0 , 0xe183b0 , None , u8"\xe1\x83\xb0" }, /* ჰ : up=ჰ : lo=ჰ */
  { 0xe183b1 , 0xe183b1 , 0xe183b1 , None , u8"\xe1\x83\xb1" }, /* ჱ : up=ჱ : lo=ჱ */
  { 0xe183b2 , 0xe183b2 , 0xe183b2 , None , u8"\xe1\x83\xb2" }, /* ჲ : up=ჲ : lo=ჲ */
  { 0xe183b3 , 0xe183b3 , 0xe183b3 , None , u8"\xe1\x83\xb3" }, /* ჳ : up=ჳ : lo=ჳ */
  { 0xe183b4 , 0xe183b4 , 0xe183b4 , None , u8"\xe1\x83\xb4" }, /* ჴ : up=ჴ : lo=ჴ */
  { 0xe183b5 , 0xe183b5 , 0xe183b5 , None , u8"\xe1\x83\xb5" }, /* ჵ : up=ჵ : lo=ჵ */
  { 0xe183b6 , 0xe183b6 , 0xe183b6 , None , u8"\xe1\x83\xb6" }, /* ჶ : up=ჶ : lo=ჶ */
  { 0xe183b7 , 0xe183b7 , 0xe183b7 , None , u8"\xe1\x83\xb7" }, /* ჷ : up=ჷ : lo=ჷ */
  { 0xe183b8 , 0xe183b8 , 0xe183b8 , None , u8"\xe1\x83\xb8" }, /* ჸ : up=ჸ : lo=ჸ */
  { 0xe183b9 , 0xe183b9 , 0xe183b9 , None , u8"\xe1\x83\xb9" }, /* ჹ : up=ჹ : lo=ჹ */
  { 0xe183ba , 0xe183ba , 0xe183ba , None , u8"\xe1\x83\xba" }, /* ჺ : up=ჺ : lo=ჺ */
  { 0xe183bb , 0xe183bb , 0xe183bb , None , u8"\xe1\x83\xbb" }, /* ჻ : up=჻ : lo=჻ */
  { 0xe183bc , 0xe183bc , 0xe183bc , None , u8"\xe1\x83\xbc" }, /* ჼ : up=ჼ : lo=ჼ */
  { 0xe183bd , 0xe183bd , 0xe183bd , None , u8"\xe1\x83\xbd" },
  { 0xe183be , 0xe183be , 0xe183be , None , u8"\xe1\x83\xbe" },
  { 0xe183bf , 0xe183bf , 0xe183bf , None , u8"\xe1\x83\xbf" },
};

const character charmap_e1_b8[64] = {
  { 0xe1b880 , 0xe1b880 , 0xe1b881 , None , u8"\xe1\xb8\x80" }, /* Ḁ : up=Ḁ : lo=ḁ */
  { 0xe1b881 , 0xe1b880 , 0xe1b881 , None , u8"\xe1\xb8\x81" }, /* ḁ : up=Ḁ : lo=ḁ */
  { 0xe1b882 , 0xe1b882 , 0xe1b883 , None , u8"\xe1\xb8\x82" }, /* Ḃ : up=Ḃ : lo=ḃ */
  { 0xe1b883 , 0xe1b882 , 0xe1b883 , None , u8"\xe1\xb8\x83" }, /* ḃ : up=Ḃ : lo=ḃ */
  { 0xe1b884 , 0xe1b884 , 0xe1b885 , None , u8"\xe1\xb8\x84" }, /* Ḅ : up=Ḅ : lo=ḅ */
  { 0xe1b885 , 0xe1b884 , 0xe1b885 , None , u8"\xe1\xb8\x85" }, /* ḅ : up=Ḅ : lo=ḅ */
  { 0xe1b886 , 0xe1b886 , 0xe1b887 , None , u8"\xe1\xb8\x86" }, /* Ḇ : up=Ḇ : lo=ḇ */
  { 0xe1b887 , 0xe1b886 , 0xe1b887 , None , u8"\xe1\xb8\x87" }, /* ḇ : up=Ḇ : lo=ḇ */
  { 0xe1b888 , 0xe1b888 , 0xe1b889 , None , u8"\xe1\xb8\x88" }, /* Ḉ : up=Ḉ : lo=ḉ */
  { 0xe1b889 , 0xe1b888 , 0xe1b889 , None , u8"\xe1\xb8\x89" }, /* ḉ : up=Ḉ : lo=ḉ */
  { 0xe1b88a , 0xe1b88a , 0xe1b88b , None , u8"\xe1\xb8\x8a" }, /* Ḋ : up=Ḋ : lo=ḋ */
  { 0xe1b88b , 0xe1b88a , 0xe1b88b , None , u8"\xe1\xb8\x8b" }, /* ḋ : up=Ḋ : lo=ḋ */
  { 0xe1b88c , 0xe1b88c , 0xe1b88d , None , u8"\xe1\xb8\x8c" }, /* Ḍ : up=Ḍ : lo=ḍ */
  { 0xe1b88d , 0xe1b88c , 0xe1b88d , None , u8"\xe1\xb8\x8d" }, /* ḍ : up=Ḍ : lo=ḍ */
  { 0xe1b88e , 0xe1b88e , 0xe1b88f , None , u8"\xe1\xb8\x8e" }, /* Ḏ : up=Ḏ : lo=ḏ */
  { 0xe1b88f , 0xe1b88e , 0xe1b88f , None , u8"\xe1\xb8\x8f" }, /* ḏ : up=Ḏ : lo=ḏ */
  { 0xe1b890 , 0xe1b890 , 0xe1b891 , None , u8"\xe1\xb8\x90" }, /* Ḑ : up=Ḑ : lo=ḑ */
  { 0xe1b891 , 0xe1b890 , 0xe1b891 , None , u8"\xe1\xb8\x91" }, /* ḑ : up=Ḑ : lo=ḑ */
  { 0xe1b892 , 0xe1b892 , 0xe1b893 , None , u8"\xe1\xb8\x92" }, /* Ḓ : up=Ḓ : lo=ḓ */
  { 0xe1b893 , 0xe1b892 , 0xe1b893 , None , u8"\xe1\xb8\x93" }, /* ḓ : up=Ḓ : lo=ḓ */
  { 0xe1b894 , 0xe1b894 , 0xe1b895 , None , u8"\xe1\xb8\x94" }, /* Ḕ : up=Ḕ : lo=ḕ */
  { 0xe1b895 , 0xe1b894 , 0xe1b895 , None , u8"\xe1\xb8\x95" }, /* ḕ : up=Ḕ : lo=ḕ */
  { 0xe1b896 , 0xe1b896 , 0xe1b897 , None , u8"\xe1\xb8\x96" }, /* Ḗ : up=Ḗ : lo=ḗ */
  { 0xe1b897 , 0xe1b896 , 0xe1b897 , None , u8"\xe1\xb8\x97" }, /* ḗ : up=Ḗ : lo=ḗ */
  { 0xe1b898 , 0xe1b898 , 0xe1b899 , None , u8"\xe1\xb8\x98" }, /* Ḙ : up=Ḙ : lo=ḙ */
  { 0xe1b899 , 0xe1b898 , 0xe1b899 , None , u8"\xe1\xb8\x99" }, /* ḙ : up=Ḙ : lo=ḙ */
  { 0xe1b89a , 0xe1b89a , 0xe1b89b , None , u8"\xe1\xb8\x9a" }, /* Ḛ : up=Ḛ : lo=ḛ */
  { 0xe1b89b , 0xe1b89a , 0xe1b89b , None , u8"\xe1\xb8\x9b" }, /* ḛ : up=Ḛ : lo=ḛ */
  { 0xe1b89c , 0xe1b89c , 0xe1b89d , None , u8"\xe1\xb8\x9c" }, /* Ḝ : up=Ḝ : lo=ḝ */
  { 0xe1b89d , 0xe1b89c , 0xe1b89d , None , u8"\xe1\xb8\x9d" }, /* ḝ : up=Ḝ : lo=ḝ */
  { 0xe1b89e , 0xe1b89e , 0xe1b89f , None , u8"\xe1\xb8\x9e" }, /* Ḟ : up=Ḟ : lo=ḟ */
  { 0xe1b89f , 0xe1b89e , 0xe1b89f , None , u8"\xe1\xb8\x9f" }, /* ḟ : up=Ḟ : lo=ḟ */
  { 0xe1b8a0 , 0xe1b8a0 , 0xe1b8a1 , None , u8"\xe1\xb8\xa0" }, /* Ḡ : up=Ḡ : lo=ḡ */
  { 0xe1b8a1 , 0xe1b8a0 , 0xe1b8a1 , None , u8"\xe1\xb8\xa1" }, /* ḡ : up=Ḡ : lo=ḡ */
  { 0xe1b8a2 , 0xe1b8a2 , 0xe1b8a3 , None , u8"\xe1\xb8\xa2" }, /* Ḣ : up=Ḣ : lo=ḣ */
  { 0xe1b8a3 , 0xe1b8a2 , 0xe1b8a3 , None , u8"\xe1\xb8\xa3" }, /* ḣ : up=Ḣ : lo=ḣ */
  { 0xe1b8a4 , 0xe1b8a4 , 0xe1b8a5 , None , u8"\xe1\xb8\xa4" }, /* Ḥ : up=Ḥ : lo=ḥ */
  { 0xe1b8a5 , 0xe1b8a4 , 0xe1b8a5 , None , u8"\xe1\xb8\xa5" }, /* ḥ : up=Ḥ : lo=ḥ */
  { 0xe1b8a6 , 0xe1b8a6 , 0xe1b8a7 , None , u8"\xe1\xb8\xa6" }, /* Ḧ : up=Ḧ : lo=ḧ */
  { 0xe1b8a7 , 0xe1b8a6 , 0xe1b8a7 , None , u8"\xe1\xb8\xa7" }, /* ḧ : up=Ḧ : lo=ḧ */
  { 0xe1b8a8 , 0xe1b8a8 , 0xe1b8a9 , None , u8"\xe1\xb8\xa8" }, /* Ḩ : up=Ḩ : lo=ḩ */
  { 0xe1b8a9 , 0xe1b8a8 , 0xe1b8a9 , None , u8"\xe1\xb8\xa9" }, /* ḩ : up=Ḩ : lo=ḩ */
  { 0xe1b8aa , 0xe1b8aa , 0xe1b8ab , None , u8"\xe1\xb8\xaa" }, /* Ḫ : up=Ḫ : lo=ḫ */
  { 0xe1b8ab , 0xe1b8aa , 0xe1b8ab , None , u8"\xe1\xb8\xab" }, /* ḫ : up=Ḫ : lo=ḫ */
  { 0xe1b8ac , 0xe1b8ac , 0xe1b8ad , None , u8"\xe1\xb8\xac" }, /* Ḭ : up=Ḭ : lo=ḭ */
  { 0xe1b8ad , 0xe1b8ac , 0xe1b8ad , None , u8"\xe1\xb8\xad" }, /* ḭ : up=Ḭ : lo=ḭ */
  { 0xe1b8ae , 0xe1b8ae , 0xe1b8af , None , u8"\xe1\xb8\xae" }, /* Ḯ : up=Ḯ : lo=ḯ */
  { 0xe1b8af , 0xe1b8ae , 0xe1b8af , None , u8"\xe1\xb8\xaf" }, /* ḯ : up=Ḯ : lo=ḯ */
  { 0xe1b8b0 , 0xe1b8b0 , 0xe1b8b1 , None , u8"\xe1\xb8\xb0" }, /* Ḱ : up=Ḱ : lo=ḱ */
  { 0xe1b8b1 , 0xe1b8b0 , 0xe1b8b1 , None , u8"\xe1\xb8\xb1" }, /* ḱ : up=Ḱ : lo=ḱ */
  { 0xe1b8b2 , 0xe1b8b2 , 0xe1b8b3 , None , u8"\xe1\xb8\xb2" }, /* Ḳ : up=Ḳ : lo=ḳ */
  { 0xe1b8b3 , 0xe1b8b2 , 0xe1b8b3 , None , u8"\xe1\xb8\xb3" }, /* ḳ : up=Ḳ : lo=ḳ */
  { 0xe1b8b4 , 0xe1b8b4 , 0xe1b8b5 , None , u8"\xe1\xb8\xb4" }, /* Ḵ : up=Ḵ : lo=ḵ */
  { 0xe1b8b5 , 0xe1b8b4 , 0xe1b8b5 , None , u8"\xe1\xb8\xb5" }, /* ḵ : up=Ḵ : lo=ḵ */
  { 0xe1b8b6 , 0xe1b8b6 , 0xe1b8b7 , None , u8"\xe1\xb8\xb6" }, /* Ḷ : up=Ḷ : lo=ḷ */
  { 0xe1b8b7 , 0xe1b8b6 , 0xe1b8b7 , None , u8"\xe1\xb8\xb7" }, /* ḷ : up=Ḷ : lo=ḷ */
  { 0xe1b8b8 , 0xe1b8b8 , 0xe1b8b9 , None , u8"\xe1\xb8\xb8" }, /* Ḹ : up=Ḹ : lo=ḹ */
  { 0xe1b8b9 , 0xe1b8b8 , 0xe1b8b9 , None , u8"\xe1\xb8\xb9" }, /* ḹ : up=Ḹ : lo=ḹ */
  { 0xe1b8ba , 0xe1b8ba , 0xe1b8bb , None , u8"\xe1\xb8\xba" }, /* Ḻ : up=Ḻ : lo=ḻ */
  { 0xe1b8bb , 0xe1b8ba , 0xe1b8bb , None , u8"\xe1\xb8\xbb" }, /* ḻ : up=Ḻ : lo=ḻ */
  { 0xe1b8bc , 0xe1b8bc , 0xe1b8bd , None , u8"\xe1\xb8\xbc" }, /* Ḽ : up=Ḽ : lo=ḽ */
  { 0xe1b8bd , 0xe1b8bc , 0xe1b8bd , None , u8"\xe1\xb8\xbd" }, /* ḽ : up=Ḽ : lo=ḽ */
  { 0xe1b8be , 0xe1b8be , 0xe1b8bf , None , u8"\xe1\xb8\xbe" }, /* Ḿ : up=Ḿ : lo=ḿ */
  { 0xe1b8bf , 0xe1b8be , 0xe1b8bf , None , u8"\xe1\xb8\xbf" }, /* ḿ : up=Ḿ : lo=ḿ */
};

const character charmap_e1_b9[64] = {
  { 0xe1b980 , 0xe1b980 , 0xe1b981 , None , u8"\xe1\xb9\x80" }, /* Ṁ : up=Ṁ : lo=ṁ */
  { 0xe1b981 , 0xe1b980 , 0xe1b981 , None , u8"\xe1\xb9\x81" }, /* ṁ : up=Ṁ : lo=ṁ */
  { 0xe1b982 , 0xe1b982 , 0xe1b983 , None , u8"\xe1\xb9\x82" }, /* Ṃ : up=Ṃ : lo=ṃ */
  { 0xe1b983 , 0xe1b982 , 0xe1b983 , None , u8"\xe1\xb9\x83" }, /* ṃ : up=Ṃ : lo=ṃ */
  { 0xe1b984 , 0xe1b984 , 0xe1b985 , None , u8"\xe1\xb9\x84" }, /* Ṅ : up=Ṅ : lo=ṅ */
  { 0xe1b985 , 0xe1b984 , 0xe1b985 , None , u8"\xe1\xb9\x85" }, /* ṅ : up=Ṅ : lo=ṅ */
  { 0xe1b986 , 0xe1b986 , 0xe1b987 , None , u8"\xe1\xb9\x86" }, /* Ṇ : up=Ṇ : lo=ṇ */
  { 0xe1b987 , 0xe1b986 , 0xe1b987 , None , u8"\xe1\xb9\x87" }, /* ṇ : up=Ṇ : lo=ṇ */
  { 0xe1b988 , 0xe1b988 , 0xe1b989 , None , u8"\xe1\xb9\x88" }, /* Ṉ : up=Ṉ : lo=ṉ */
  { 0xe1b989 , 0xe1b988 , 0xe1b989 , None , u8"\xe1\xb9\x89" }, /* ṉ : up=Ṉ : lo=ṉ */
  { 0xe1b98a , 0xe1b98a , 0xe1b98b , None , u8"\xe1\xb9\x8a" }, /* Ṋ : up=Ṋ : lo=ṋ */
  { 0xe1b98b , 0xe1b98a , 0xe1b98b , None , u8"\xe1\xb9\x8b" }, /* ṋ : up=Ṋ : lo=ṋ */
  { 0xe1b98c , 0xe1b98c , 0xe1b98d , None , u8"\xe1\xb9\x8c" }, /* Ṍ : up=Ṍ : lo=ṍ */
  { 0xe1b98d , 0xe1b98c , 0xe1b98d , None , u8"\xe1\xb9\x8d" }, /* ṍ : up=Ṍ : lo=ṍ */
  { 0xe1b98e , 0xe1b98e , 0xe1b98f , None , u8"\xe1\xb9\x8e" }, /* Ṏ : up=Ṏ : lo=ṏ */
  { 0xe1b98f , 0xe1b98e , 0xe1b98f , None , u8"\xe1\xb9\x8f" }, /* ṏ : up=Ṏ : lo=ṏ */
  { 0xe1b990 , 0xe1b990 , 0xe1b991 , None , u8"\xe1\xb9\x90" }, /* Ṑ : up=Ṑ : lo=ṑ */
  { 0xe1b991 , 0xe1b990 , 0xe1b991 , None , u8"\xe1\xb9\x91" }, /* ṑ : up=Ṑ : lo=ṑ */
  { 0xe1b992 , 0xe1b992 , 0xe1b993 , None , u8"\xe1\xb9\x92" }, /* Ṓ : up=Ṓ : lo=ṓ */
  { 0xe1b993 , 0xe1b992 , 0xe1b993 , None , u8"\xe1\xb9\x93" }, /* ṓ : up=Ṓ : lo=ṓ */
  { 0xe1b994 , 0xe1b994 , 0xe1b995 , None , u8"\xe1\xb9\x94" }, /* Ṕ : up=Ṕ : lo=ṕ */
  { 0xe1b995 , 0xe1b994 , 0xe1b995 , None , u8"\xe1\xb9\x95" }, /* ṕ : up=Ṕ : lo=ṕ */
  { 0xe1b996 , 0xe1b996 , 0xe1b997 , None , u8"\xe1\xb9\x96" }, /* Ṗ : up=Ṗ : lo=ṗ */
  { 0xe1b997 , 0xe1b996 , 0xe1b997 , None , u8"\xe1\xb9\x97" }, /* ṗ : up=Ṗ : lo=ṗ */
  { 0xe1b998 , 0xe1b998 , 0xe1b999 , None , u8"\xe1\xb9\x98" }, /* Ṙ : up=Ṙ : lo=ṙ */
  { 0xe1b999 , 0xe1b998 , 0xe1b999 , None , u8"\xe1\xb9\x99" }, /* ṙ : up=Ṙ : lo=ṙ */
  { 0xe1b99a , 0xe1b99a , 0xe1b99b , None , u8"\xe1\xb9\x9a" }, /* Ṛ : up=Ṛ : lo=ṛ */
  { 0xe1b99b , 0xe1b99a , 0xe1b99b , None , u8"\xe1\xb9\x9b" }, /* ṛ : up=Ṛ : lo=ṛ */
  { 0xe1b99c , 0xe1b99c , 0xe1b99d , None , u8"\xe1\xb9\x9c" }, /* Ṝ : up=Ṝ : lo=ṝ */
  { 0xe1b99d , 0xe1b99c , 0xe1b99d , None , u8"\xe1\xb9\x9d" }, /* ṝ : up=Ṝ : lo=ṝ */
  { 0xe1b99e , 0xe1b99e , 0xe1b99f , None , u8"\xe1\xb9\x9e" }, /* Ṟ : up=Ṟ : lo=ṟ */
  { 0xe1b99f , 0xe1b99e , 0xe1b99f , None , u8"\xe1\xb9\x9f" }, /* ṟ : up=Ṟ : lo=ṟ */
  { 0xe1b9a0 , 0xe1b9a0 , 0xe1b9a1 , None , u8"\xe1\xb9\xa0" }, /* Ṡ : up=Ṡ : lo=ṡ */
  { 0xe1b9a1 , 0xe1b9a0 , 0xe1b9a1 , None , u8"\xe1\xb9\xa1" }, /* ṡ : up=Ṡ : lo=ṡ */
  { 0xe1b9a2 , 0xe1b9a2 , 0xe1b9a3 , None , u8"\xe1\xb9\xa2" }, /* Ṣ : up=Ṣ : lo=ṣ */
  { 0xe1b9a3 , 0xe1b9a2 , 0xe1b9a3 , None , u8"\xe1\xb9\xa3" }, /* ṣ : up=Ṣ : lo=ṣ */
  { 0xe1b9a4 , 0xe1b9a4 , 0xe1b9a5 , None , u8"\xe1\xb9\xa4" }, /* Ṥ : up=Ṥ : lo=ṥ */
  { 0xe1b9a5 , 0xe1b9a4 , 0xe1b9a5 , None , u8"\xe1\xb9\xa5" }, /* ṥ : up=Ṥ : lo=ṥ */
  { 0xe1b9a6 , 0xe1b9a6 , 0xe1b9a7 , None , u8"\xe1\xb9\xa6" }, /* Ṧ : up=Ṧ : lo=ṧ */
  { 0xe1b9a7 , 0xe1b9a6 , 0xe1b9a7 , None , u8"\xe1\xb9\xa7" }, /* ṧ : up=Ṧ : lo=ṧ */
  { 0xe1b9a8 , 0xe1b9a8 , 0xe1b9a9 , None , u8"\xe1\xb9\xa8" }, /* Ṩ : up=Ṩ : lo=ṩ */
  { 0xe1b9a9 , 0xe1b9a8 , 0xe1b9a9 , None , u8"\xe1\xb9\xa9" }, /* ṩ : up=Ṩ : lo=ṩ */
  { 0xe1b9aa , 0xe1b9aa , 0xe1b9ab , None , u8"\xe1\xb9\xaa" }, /* Ṫ : up=Ṫ : lo=ṫ */
  { 0xe1b9ab , 0xe1b9aa , 0xe1b9ab , None , u8"\xe1\xb9\xab" }, /* ṫ : up=Ṫ : lo=ṫ */
  { 0xe1b9ac , 0xe1b9ac , 0xe1b9ad , None , u8"\xe1\xb9\xac" }, /* Ṭ : up=Ṭ : lo=ṭ */
  { 0xe1b9ad , 0xe1b9ac , 0xe1b9ad , None , u8"\xe1\xb9\xad" }, /* ṭ : up=Ṭ : lo=ṭ */
  { 0xe1b9ae , 0xe1b9ae , 0xe1b9af , None , u8"\xe1\xb9\xae" }, /* Ṯ : up=Ṯ : lo=ṯ */
  { 0xe1b9af , 0xe1b9ae , 0xe1b9af , None , u8"\xe1\xb9\xaf" }, /* ṯ : up=Ṯ : lo=ṯ */
  { 0xe1b9b0 , 0xe1b9b0 , 0xe1b9b1 , None , u8"\xe1\xb9\xb0" }, /* Ṱ : up=Ṱ : lo=ṱ */
  { 0xe1b9b1 , 0xe1b9b0 , 0xe1b9b1 , None , u8"\xe1\xb9\xb1" }, /* ṱ : up=Ṱ : lo=ṱ */
  { 0xe1b9b2 , 0xe1b9b2 , 0xe1b9b3 , None , u8"\xe1\xb9\xb2" }, /* Ṳ : up=Ṳ : lo=ṳ */
  { 0xe1b9b3 , 0xe1b9b2 , 0xe1b9b3 , None , u8"\xe1\xb9\xb3" }, /* ṳ : up=Ṳ : lo=ṳ */
  { 0xe1b9b4 , 0xe1b9b4 , 0xe1b9b5 , None , u8"\xe1\xb9\xb4" }, /* Ṵ : up=Ṵ : lo=ṵ */
  { 0xe1b9b5 , 0xe1b9b4 , 0xe1b9b5 , None , u8"\xe1\xb9\xb5" }, /* ṵ : up=Ṵ : lo=ṵ */
  { 0xe1b9b6 , 0xe1b9b6 , 0xe1b9b7 , None , u8"\xe1\xb9\xb6" }, /* Ṷ : up=Ṷ : lo=ṷ */
  { 0xe1b9b7 , 0xe1b9b6 , 0xe1b9b7 , None , u8"\xe1\xb9\xb7" }, /* ṷ : up=Ṷ : lo=ṷ */
  { 0xe1b9b8 , 0xe1b9b8 , 0xe1b9b9 , None , u8"\xe1\xb9\xb8" }, /* Ṹ : up=Ṹ : lo=ṹ */
  { 0xe1b9b9 , 0xe1b9b8 , 0xe1b9b9 , None , u8"\xe1\xb9\xb9" }, /* ṹ : up=Ṹ : lo=ṹ */
  { 0xe1b9ba , 0xe1b9ba , 0xe1b9bb , None , u8"\xe1\xb9\xba" }, /* Ṻ : up=Ṻ : lo=ṻ */
  { 0xe1b9bb , 0xe1b9ba , 0xe1b9bb , None , u8"\xe1\xb9\xbb" }, /* ṻ : up=Ṻ : lo=ṻ */
  { 0xe1b9bc , 0xe1b9bc , 0xe1b9bd , None , u8"\xe1\xb9\xbc" }, /* Ṽ : up=Ṽ : lo=ṽ */
  { 0xe1b9bd , 0xe1b9bc , 0xe1b9bd , None , u8"\xe1\xb9\xbd" }, /* ṽ : up=Ṽ : lo=ṽ */
  { 0xe1b9be , 0xe1b9be , 0xe1b9bf , None , u8"\xe1\xb9\xbe" }, /* Ṿ : up=Ṿ : lo=ṿ */
  { 0xe1b9bf , 0xe1b9be , 0xe1b9bf , None , u8"\xe1\xb9\xbf" }, /* ṿ : up=Ṿ : lo=ṿ */
};

const character charmap_e1_ba[64] = {
  { 0xe1ba80 , 0xe1ba80 , 0xe1ba81 , None , u8"\xe1\xba\x80" }, /* Ẁ : up=Ẁ : lo=ẁ */
  { 0xe1ba81 , 0xe1ba80 , 0xe1ba81 , None , u8"\xe1\xba\x81" }, /* ẁ : up=Ẁ : lo=ẁ */
  { 0xe1ba82 , 0xe1ba82 , 0xe1ba83 , None , u8"\xe1\xba\x82" }, /* Ẃ : up=Ẃ : lo=ẃ */
  { 0xe1ba83 , 0xe1ba82 , 0xe1ba83 , None , u8"\xe1\xba\x83" }, /* ẃ : up=Ẃ : lo=ẃ */
  { 0xe1ba84 , 0xe1ba84 , 0xe1ba85 , None , u8"\xe1\xba\x84" }, /* Ẅ : up=Ẅ : lo=ẅ */
  { 0xe1ba85 , 0xe1ba84 , 0xe1ba85 , None , u8"\xe1\xba\x85" }, /* ẅ : up=Ẅ : lo=ẅ */
  { 0xe1ba86 , 0xe1ba86 , 0xe1ba87 , None , u8"\xe1\xba\x86" }, /* Ẇ : up=Ẇ : lo=ẇ */
  { 0xe1ba87 , 0xe1ba86 , 0xe1ba87 , None , u8"\xe1\xba\x87" }, /* ẇ : up=Ẇ : lo=ẇ */
  { 0xe1ba88 , 0xe1ba88 , 0xe1ba89 , None , u8"\xe1\xba\x88" }, /* Ẉ : up=Ẉ : lo=ẉ */
  { 0xe1ba89 , 0xe1ba88 , 0xe1ba89 , None , u8"\xe1\xba\x89" }, /* ẉ : up=Ẉ : lo=ẉ */
  { 0xe1ba8a , 0xe1ba8a , 0xe1ba8b , None , u8"\xe1\xba\x8a" }, /* Ẋ : up=Ẋ : lo=ẋ */
  { 0xe1ba8b , 0xe1ba8a , 0xe1ba8b , None , u8"\xe1\xba\x8b" }, /* ẋ : up=Ẋ : lo=ẋ */
  { 0xe1ba8c , 0xe1ba8c , 0xe1ba8d , None , u8"\xe1\xba\x8c" }, /* Ẍ : up=Ẍ : lo=ẍ */
  { 0xe1ba8d , 0xe1ba8c , 0xe1ba8d , None , u8"\xe1\xba\x8d" }, /* ẍ : up=Ẍ : lo=ẍ */
  { 0xe1ba8e , 0xe1ba8e , 0xe1ba8f , None , u8"\xe1\xba\x8e" }, /* Ẏ : up=Ẏ : lo=ẏ */
  { 0xe1ba8f , 0xe1ba8e , 0xe1ba8f , None , u8"\xe1\xba\x8f" }, /* ẏ : up=Ẏ : lo=ẏ */
  { 0xe1ba90 , 0xe1ba90 , 0xe1ba91 , None , u8"\xe1\xba\x90" }, /* Ẑ : up=Ẑ : lo=ẑ */
  { 0xe1ba91 , 0xe1ba90 , 0xe1ba91 , None , u8"\xe1\xba\x91" }, /* ẑ : up=Ẑ : lo=ẑ */
  { 0xe1ba92 , 0xe1ba92 , 0xe1ba93 , None , u8"\xe1\xba\x92" }, /* Ẓ : up=Ẓ : lo=ẓ */
  { 0xe1ba93 , 0xe1ba92 , 0xe1ba93 , None , u8"\xe1\xba\x93" }, /* ẓ : up=Ẓ : lo=ẓ */
  { 0xe1ba94 , 0xe1ba94 , 0xe1ba95 , None , u8"\xe1\xba\x94" }, /* Ẕ : up=Ẕ : lo=ẕ */
  { 0xe1ba95 , 0xe1ba94 , 0xe1ba95 , None , u8"\xe1\xba\x95" }, /* ẕ : up=Ẕ : lo=ẕ */
  { 0xe1ba96 , 0xe1ba96 , 0xe1ba96 , None , u8"\xe1\xba\x96" }, /* ẖ : up=ẖ : lo=ẖ */
  { 0xe1ba97 , 0xe1ba97 , 0xe1ba97 , None , u8"\xe1\xba\x97" }, /* ẗ : up=ẗ : lo=ẗ */
  { 0xe1ba98 , 0xe1ba98 , 0xe1ba98 , None , u8"\xe1\xba\x98" }, /* ẘ : up=ẘ : lo=ẘ */
  { 0xe1ba99 , 0xe1ba99 , 0xe1ba99 , None , u8"\xe1\xba\x99" }, /* ẙ : up=ẙ : lo=ẙ */
  { 0xe1ba9a , 0xe1ba9a , 0xe1ba9a , None , u8"\xe1\xba\x9a" }, /* ẚ : up=ẚ : lo=ẚ */
  { 0xe1ba9b , 0xe1b9a0 , 0xe1ba9b , None , u8"\xe1\xba\x9b" }, /* ẛ : up=Ṡ : lo=ẛ */
  { 0xe1ba9c , 0xe1ba9c , 0xe1ba9c , None , u8"\xe1\xba\x9c" }, /* ẜ : up=ẜ : lo=ẜ */
  { 0xe1ba9d , 0xe1ba9d , 0xe1ba9d , None , u8"\xe1\xba\x9d" }, /* ẝ : up=ẝ : lo=ẝ */
  { 0xe1ba9e , 0xe1ba9e , 0xc39f , None , u8"\xe1\xba\x9e" }, /* ẞ : up=ẞ : lo=ß */
  { 0xe1ba9f , 0xe1ba9f , 0xe1ba9f , None , u8"\xe1\xba\x9f" }, /* ẟ : up=ẟ : lo=ẟ */
  { 0xe1baa0 , 0xe1baa0 , 0xe1baa1 , None , u8"\xe1\xba\xa0" }, /* Ạ : up=Ạ : lo=ạ */
  { 0xe1baa1 , 0xe1baa0 , 0xe1baa1 , None , u8"\xe1\xba\xa1" }, /* ạ : up=Ạ : lo=ạ */
  { 0xe1baa2 , 0xe1baa2 , 0xe1baa3 , None , u8"\xe1\xba\xa2" }, /* Ả : up=Ả : lo=ả */
  { 0xe1baa3 , 0xe1baa2 , 0xe1baa3 , None , u8"\xe1\xba\xa3" }, /* ả : up=Ả : lo=ả */
  { 0xe1baa4 , 0xe1baa4 , 0xe1baa5 , None , u8"\xe1\xba\xa4" }, /* Ấ : up=Ấ : lo=ấ */
  { 0xe1baa5 , 0xe1baa4 , 0xe1baa5 , None , u8"\xe1\xba\xa5" }, /* ấ : up=Ấ : lo=ấ */
  { 0xe1baa6 , 0xe1baa6 , 0xe1baa7 , None , u8"\xe1\xba\xa6" }, /* Ầ : up=Ầ : lo=ầ */
  { 0xe1baa7 , 0xe1baa6 , 0xe1baa7 , None , u8"\xe1\xba\xa7" }, /* ầ : up=Ầ : lo=ầ */
  { 0xe1baa8 , 0xe1baa8 , 0xe1baa9 , None , u8"\xe1\xba\xa8" }, /* Ẩ : up=Ẩ : lo=ẩ */
  { 0xe1baa9 , 0xe1baa8 , 0xe1baa9 , None , u8"\xe1\xba\xa9" }, /* ẩ : up=Ẩ : lo=ẩ */
  { 0xe1baaa , 0xe1baaa , 0xe1baab , None , u8"\xe1\xba\xaa" }, /* Ẫ : up=Ẫ : lo=ẫ */
  { 0xe1baab , 0xe1baaa , 0xe1baab , None , u8"\xe1\xba\xab" }, /* ẫ : up=Ẫ : lo=ẫ */
  { 0xe1baac , 0xe1baac , 0xe1baad , None , u8"\xe1\xba\xac" }, /* Ậ : up=Ậ : lo=ậ */
  { 0xe1baad , 0xe1baac , 0xe1baad , None , u8"\xe1\xba\xad" }, /* ậ : up=Ậ : lo=ậ */
  { 0xe1baae , 0xe1baae , 0xe1baaf , None , u8"\xe1\xba\xae" }, /* Ắ : up=Ắ : lo=ắ */
  { 0xe1baaf , 0xe1baae , 0xe1baaf , None , u8"\xe1\xba\xaf" }, /* ắ : up=Ắ : lo=ắ */
  { 0xe1bab0 , 0xe1bab0 , 0xe1bab1 , None , u8"\xe1\xba\xb0" }, /* Ằ : up=Ằ : lo=ằ */
  { 0xe1bab1 , 0xe1bab0 , 0xe1bab1 , None , u8"\xe1\xba\xb1" }, /* ằ : up=Ằ : lo=ằ */
  { 0xe1bab2 , 0xe1bab2 , 0xe1bab3 , None , u8"\xe1\xba\xb2" }, /* Ẳ : up=Ẳ : lo=ẳ */
  { 0xe1bab3 , 0xe1bab2 , 0xe1bab3 , None , u8"\xe1\xba\xb3" }, /* ẳ : up=Ẳ : lo=ẳ */
  { 0xe1bab4 , 0xe1bab4 , 0xe1bab5 , None , u8"\xe1\xba\xb4" }, /* Ẵ : up=Ẵ : lo=ẵ */
  { 0xe1bab5 , 0xe1bab4 , 0xe1bab5 , None , u8"\xe1\xba\xb5" }, /* ẵ : up=Ẵ : lo=ẵ */
  { 0xe1bab6 , 0xe1bab6 , 0xe1bab7 , None , u8"\xe1\xba\xb6" }, /* Ặ : up=Ặ : lo=ặ */
  { 0xe1bab7 , 0xe1bab6 , 0xe1bab7 , None , u8"\xe1\xba\xb7" }, /* ặ : up=Ặ : lo=ặ */
  { 0xe1bab8 , 0xe1bab8 , 0xe1bab9 , None , u8"\xe1\xba\xb8" }, /* Ẹ : up=Ẹ : lo=ẹ */
  { 0xe1bab9 , 0xe1bab8 , 0xe1bab9 , None , u8"\xe1\xba\xb9" }, /* ẹ : up=Ẹ : lo=ẹ */
  { 0xe1baba , 0xe1baba , 0xe1babb , None , u8"\xe1\xba\xba" }, /* Ẻ : up=Ẻ : lo=ẻ */
  { 0xe1babb , 0xe1baba , 0xe1babb , None , u8"\xe1\xba\xbb" }, /* ẻ : up=Ẻ : lo=ẻ */
  { 0xe1babc , 0xe1babc , 0xe1babd , None , u8"\xe1\xba\xbc" }, /* Ẽ : up=Ẽ : lo=ẽ */
  { 0xe1babd , 0xe1babc , 0xe1babd , None , u8"\xe1\xba\xbd" }, /* ẽ : up=Ẽ : lo=ẽ */
  { 0xe1babe , 0xe1babe , 0xe1babf , None , u8"\xe1\xba\xbe" }, /* Ế : up=Ế : lo=ế */
  { 0xe1babf , 0xe1babe , 0xe1babf , None , u8"\xe1\xba\xbf" }, /* ế : up=Ế : lo=ế */
};

const character charmap_e1_bb[64] = {
  { 0xe1bb80 , 0xe1bb80 , 0xe1bb81 , None , u8"\xe1\xbb\x80" }, /* Ề : up=Ề : lo=ề */
  { 0xe1bb81 , 0xe1bb80 , 0xe1bb81 , None , u8"\xe1\xbb\x81" }, /* ề : up=Ề : lo=ề */
  { 0xe1bb82 , 0xe1bb82 , 0xe1bb83 , None , u8"\xe1\xbb\x82" }, /* Ể : up=Ể : lo=ể */
  { 0xe1bb83 , 0xe1bb82 , 0xe1bb83 , None , u8"\xe1\xbb\x83" }, /* ể : up=Ể : lo=ể */
  { 0xe1bb84 , 0xe1bb84 , 0xe1bb85 , None , u8"\xe1\xbb\x84" }, /* Ễ : up=Ễ : lo=ễ */
  { 0xe1bb85 , 0xe1bb84 , 0xe1bb85 , None , u8"\xe1\xbb\x85" }, /* ễ : up=Ễ : lo=ễ */
  { 0xe1bb86 , 0xe1bb86 , 0xe1bb87 , None , u8"\xe1\xbb\x86" }, /* Ệ : up=Ệ : lo=ệ */
  { 0xe1bb87 , 0xe1bb86 , 0xe1bb87 , None , u8"\xe1\xbb\x87" }, /* ệ : up=Ệ : lo=ệ */
  { 0xe1bb88 , 0xe1bb88 , 0xe1bb89 , None , u8"\xe1\xbb\x88" }, /* Ỉ : up=Ỉ : lo=ỉ */
  { 0xe1bb89 , 0xe1bb88 , 0xe1bb89 , None , u8"\xe1\xbb\x89" }, /* ỉ : up=Ỉ : lo=ỉ */
  { 0xe1bb8a , 0xe1bb8a , 0xe1bb8b , None , u8"\xe1\xbb\x8a" }, /* Ị : up=Ị : lo=ị */
  { 0xe1bb8b , 0xe1bb8a , 0xe1bb8b , None , u8"\xe1\xbb\x8b" }, /* ị : up=Ị : lo=ị */
  { 0xe1bb8c , 0xe1bb8c , 0xe1bb8d , None , u8"\xe1\xbb\x8c" }, /* Ọ : up=Ọ : lo=ọ */
  { 0xe1bb8d , 0xe1bb8c , 0xe1bb8d , None , u8"\xe1\xbb\x8d" }, /* ọ : up=Ọ : lo=ọ */
  { 0xe1bb8e , 0xe1bb8e , 0xe1bb8f , None , u8"\xe1\xbb\x8e" }, /* Ỏ : up=Ỏ : lo=ỏ */
  { 0xe1bb8f , 0xe1bb8e , 0xe1bb8f , None , u8"\xe1\xbb\x8f" }, /* ỏ : up=Ỏ : lo=ỏ */
  { 0xe1bb90 , 0xe1bb90 , 0xe1bb91 , None , u8"\xe1\xbb\x90" }, /* Ố : up=Ố : lo=ố */
  { 0xe1bb91 , 0xe1bb90 , 0xe1bb91 , None , u8"\xe1\xbb\x91" }, /* ố : up=Ố : lo=ố */
  { 0xe1bb92 , 0xe1bb92 , 0xe1bb93 , None , u8"\xe1\xbb\x92" }, /* Ồ : up=Ồ : lo=ồ */
  { 0xe1bb93 , 0xe1bb92 , 0xe1bb93 , None , u8"\xe1\xbb\x93" }, /* ồ : up=Ồ : lo=ồ */
  { 0xe1bb94 , 0xe1bb94 , 0xe1bb95 , None , u8"\xe1\xbb\x94" }, /* Ổ : up=Ổ : lo=ổ */
  { 0xe1bb95 , 0xe1bb94 , 0xe1bb95 , None , u8"\xe1\xbb\x95" }, /* ổ : up=Ổ : lo=ổ */
  { 0xe1bb96 , 0xe1bb96 , 0xe1bb97 , None , u8"\xe1\xbb\x96" }, /* Ỗ : up=Ỗ : lo=ỗ */
  { 0xe1bb97 , 0xe1bb96 , 0xe1bb97 , None , u8"\xe1\xbb\x97" }, /* ỗ : up=Ỗ : lo=ỗ */
  { 0xe1bb98 , 0xe1bb98 , 0xe1bb99 , None , u8"\xe1\xbb\x98" }, /* Ộ : up=Ộ : lo=ộ */
  { 0xe1bb99 , 0xe1bb98 , 0xe1bb99 , None , u8"\xe1\xbb\x99" }, /* ộ : up=Ộ : lo=ộ */
  { 0xe1bb9a , 0xe1bb9a , 0xe1bb9b , None , u8"\xe1\xbb\x9a" }, /* Ớ : up=Ớ : lo=ớ */
  { 0xe1bb9b , 0xe1bb9a , 0xe1bb9b , None , u8"\xe1\xbb\x9b" }, /* ớ : up=Ớ : lo=ớ */
  { 0xe1bb9c , 0xe1bb9c , 0xe1bb9d , None , u8"\xe1\xbb\x9c" }, /* Ờ : up=Ờ : lo=ờ */
  { 0xe1bb9d , 0xe1bb9c , 0xe1bb9d , None , u8"\xe1\xbb\x9d" }, /* ờ : up=Ờ : lo=ờ */
  { 0xe1bb9e , 0xe1bb9e , 0xe1bb9f , None , u8"\xe1\xbb\x9e" }, /* Ở : up=Ở : lo=ở */
  { 0xe1bb9f , 0xe1bb9e , 0xe1bb9f , None , u8"\xe1\xbb\x9f" }, /* ở : up=Ở : lo=ở */
  { 0xe1bba0 , 0xe1bba0 , 0xe1bba1 , None , u8"\xe1\xbb\xa0" }, /* Ỡ : up=Ỡ : lo=ỡ */
  { 0xe1bba1 , 0xe1bba0 , 0xe1bba1 , None , u8"\xe1\xbb\xa1" }, /* ỡ : up=Ỡ : lo=ỡ */
  { 0xe1bba2 , 0xe1bba2 , 0xe1bba3 , None , u8"\xe1\xbb\xa2" }, /* Ợ : up=Ợ : lo=ợ */
  { 0xe1bba3 , 0xe1bba2 , 0xe1bba3 , None , u8"\xe1\xbb\xa3" }, /* ợ : up=Ợ : lo=ợ */
  { 0xe1bba4 , 0xe1bba4 , 0xe1bba5 , None , u8"\xe1\xbb\xa4" }, /* Ụ : up=Ụ : lo=ụ */
  { 0xe1bba5 , 0xe1bba4 , 0xe1bba5 , None , u8"\xe1\xbb\xa5" }, /* ụ : up=Ụ : lo=ụ */
  { 0xe1bba6 , 0xe1bba6 , 0xe1bba7 , None , u8"\xe1\xbb\xa6" }, /* Ủ : up=Ủ : lo=ủ */
  { 0xe1bba7 , 0xe1bba6 , 0xe1bba7 , None , u8"\xe1\xbb\xa7" }, /* ủ : up=Ủ : lo=ủ */
  { 0xe1bba8 , 0xe1bba8 , 0xe1bba9 , None , u8"\xe1\xbb\xa8" }, /* Ứ : up=Ứ : lo=ứ */
  { 0xe1bba9 , 0xe1bba8 , 0xe1bba9 , None , u8"\xe1\xbb\xa9" }, /* ứ : up=Ứ : lo=ứ */
  { 0xe1bbaa , 0xe1bbaa , 0xe1bbab , None , u8"\xe1\xbb\xaa" }, /* Ừ : up=Ừ : lo=ừ */
  { 0xe1bbab , 0xe1bbaa , 0xe1bbab , None , u8"\xe1\xbb\xab" }, /* ừ : up=Ừ : lo=ừ */
  { 0xe1bbac , 0xe1bbac , 0xe1bbad , None , u8"\xe1\xbb\xac" }, /* Ử : up=Ử : lo=ử */
  { 0xe1bbad , 0xe1bbac , 0xe1bbad , None , u8"\xe1\xbb\xad" }, /* ử : up=Ử : lo=ử */
  { 0xe1bbae , 0xe1bbae , 0xe1bbaf , None , u8"\xe1\xbb\xae" }, /* Ữ : up=Ữ : lo=ữ */
  { 0xe1bbaf , 0xe1bbae , 0xe1bbaf , None , u8"\xe1\xbb\xaf" }, /* ữ : up=Ữ : lo=ữ */
  { 0xe1bbb0 , 0xe1bbb0 , 0xe1bbb1 , None , u8"\xe1\xbb\xb0" }, /* Ự : up=Ự : lo=ự */
  { 0xe1bbb1 , 0xe1bbb0 , 0xe1bbb1 , None , u8"\xe1\xbb\xb1" }, /* ự : up=Ự : lo=ự */
  { 0xe1bbb2 , 0xe1bbb2 , 0xe1bbb3 , None , u8"\xe1\xbb\xb2" }, /* Ỳ : up=Ỳ : lo=ỳ */
  { 0xe1bbb3 , 0xe1bbb2 , 0xe1bbb3 , None , u8"\xe1\xbb\xb3" }, /* ỳ : up=Ỳ : lo=ỳ */
  { 0xe1bbb4 , 0xe1bbb4 , 0xe1bbb5 , None , u8"\xe1\xbb\xb4" }, /* Ỵ : up=Ỵ : lo=ỵ */
  { 0xe1bbb5 , 0xe1bbb4 , 0xe1bbb5 , None , u8"\xe1\xbb\xb5" }, /* ỵ : up=Ỵ : lo=ỵ */
  { 0xe1bbb6 , 0xe1bbb6 , 0xe1bbb7 , None , u8"\xe1\xbb\xb6" }, /* Ỷ : up=Ỷ : lo=ỷ */
  { 0xe1bbb7 , 0xe1bbb6 , 0xe1bbb7 , None , u8"\xe1\xbb\xb7" }, /* ỷ : up=Ỷ : lo=ỷ */
  { 0xe1bbb8 , 0xe1bbb8 , 0xe1bbb9 , None , u8"\xe1\xbb\xb8" }, /* Ỹ : up=Ỹ : lo=ỹ */
  { 0xe1bbb9 , 0xe1bbb8 , 0xe1bbb9 , None , u8"\xe1\xbb\xb9" }, /* ỹ : up=Ỹ : lo=ỹ */
  { 0xe1bbba , 0xe1bbba , 0xe1bbbb , None , u8"\xe1\xbb\xba" }, /* Ỻ : up=Ỻ : lo=ỻ */
  { 0xe1bbbb , 0xe1bbba , 0xe1bbbb , None , u8"\xe1\xbb\xbb" }, /* ỻ : up=Ỻ : lo=ỻ */
  { 0xe1bbbc , 0xe1bbbc , 0xe1bbbd , None , u8"\xe1\xbb\xbc" }, /* Ỽ : up=Ỽ : lo=ỽ */
  { 0xe1bbbd , 0xe1bbbc , 0xe1bbbd , None , u8"\xe1\xbb\xbd" }, /* ỽ : up=Ỽ : lo=ỽ */
  { 0xe1bbbe , 0xe1bbbe , 0xe1bbbf , None , u8"\xe1\xbb\xbe" }, /* Ỿ : up=Ỿ : lo=ỿ */
  { 0xe1bbbf , 0xe1bbbe , 0xe1bbbf , None , u8"\xe1\xbb\xbf" }, /* ỿ : up=Ỿ : lo=ỿ */
};

const character charmap_e1_bc[64] = {
  { 0xe1bc80 , 0xe1bc88 , 0xe1bc80 , None , u8"\xe1\xbc\x80" }, /* ἀ : up=Ἀ : lo=ἀ */
  { 0xe1bc81 , 0xe1bc89 , 0xe1bc81 , None , u8"\xe1\xbc\x81" }, /* ἁ : up=Ἁ : lo=ἁ */
  { 0xe1bc82 , 0xe1bc8a , 0xe1bc82 , None , u8"\xe1\xbc\x82" }, /* ἂ : up=Ἂ : lo=ἂ */
  { 0xe1bc83 , 0xe1bc8b , 0xe1bc83 , None , u8"\xe1\xbc\x83" }, /* ἃ : up=Ἃ : lo=ἃ */
  { 0xe1bc84 , 0xe1bc8c , 0xe1bc84 , None , u8"\xe1\xbc\x84" }, /* ἄ : up=Ἄ : lo=ἄ */
  { 0xe1bc85 , 0xe1bc8d , 0xe1bc85 , None , u8"\xe1\xbc\x85" }, /* ἅ : up=Ἅ : lo=ἅ */
  { 0xe1bc86 , 0xe1bc8e , 0xe1bc86 , None , u8"\xe1\xbc\x86" }, /* ἆ : up=Ἆ : lo=ἆ */
  { 0xe1bc87 , 0xe1bc8f , 0xe1bc87 , None , u8"\xe1\xbc\x87" }, /* ἇ : up=Ἇ : lo=ἇ */
  { 0xe1bc88 , 0xe1bc88 , 0xe1bc80 , None , u8"\xe1\xbc\x88" }, /* Ἀ : up=Ἀ : lo=ἀ */
  { 0xe1bc89 , 0xe1bc89 , 0xe1bc81 , None , u8"\xe1\xbc\x89" }, /* Ἁ : up=Ἁ : lo=ἁ */
  { 0xe1bc8a , 0xe1bc8a , 0xe1bc82 , None , u8"\xe1\xbc\x8a" }, /* Ἂ : up=Ἂ : lo=ἂ */
  { 0xe1bc8b , 0xe1bc8b , 0xe1bc83 , None , u8"\xe1\xbc\x8b" }, /* Ἃ : up=Ἃ : lo=ἃ */
  { 0xe1bc8c , 0xe1bc8c , 0xe1bc84 , None , u8"\xe1\xbc\x8c" }, /* Ἄ : up=Ἄ : lo=ἄ */
  { 0xe1bc8d , 0xe1bc8d , 0xe1bc85 , None , u8"\xe1\xbc\x8d" }, /* Ἅ : up=Ἅ : lo=ἅ */
  { 0xe1bc8e , 0xe1bc8e , 0xe1bc86 , None , u8"\xe1\xbc\x8e" }, /* Ἆ : up=Ἆ : lo=ἆ */
  { 0xe1bc8f , 0xe1bc8f , 0xe1bc87 , None , u8"\xe1\xbc\x8f" }, /* Ἇ : up=Ἇ : lo=ἇ */
  { 0xe1bc90 , 0xe1bc98 , 0xe1bc90 , None , u8"\xe1\xbc\x90" }, /* ἐ : up=Ἐ : lo=ἐ */
  { 0xe1bc91 , 0xe1bc99 , 0xe1bc91 , None , u8"\xe1\xbc\x91" }, /* ἑ : up=Ἑ : lo=ἑ */
  { 0xe1bc92 , 0xe1bc9a , 0xe1bc92 , None , u8"\xe1\xbc\x92" }, /* ἒ : up=Ἒ : lo=ἒ */
  { 0xe1bc93 , 0xe1bc9b , 0xe1bc93 , None , u8"\xe1\xbc\x93" }, /* ἓ : up=Ἓ : lo=ἓ */
  { 0xe1bc94 , 0xe1bc9c , 0xe1bc94 , None , u8"\xe1\xbc\x94" }, /* ἔ : up=Ἔ : lo=ἔ */
  { 0xe1bc95 , 0xe1bc9d , 0xe1bc95 , None , u8"\xe1\xbc\x95" }, /* ἕ : up=Ἕ : lo=ἕ */
  { 0xe1bc96 , 0xe1bc96 , 0xe1bc96 , None , u8"\xe1\xbc\x96" },
  { 0xe1bc97 , 0xe1bc97 , 0xe1bc97 , None , u8"\xe1\xbc\x97" },
  { 0xe1bc98 , 0xe1bc98 , 0xe1bc90 , None , u8"\xe1\xbc\x98" }, /* Ἐ : up=Ἐ : lo=ἐ */
  { 0xe1bc99 , 0xe1bc99 , 0xe1bc91 , None , u8"\xe1\xbc\x99" }, /* Ἑ : up=Ἑ : lo=ἑ */
  { 0xe1bc9a , 0xe1bc9a , 0xe1bc92 , None , u8"\xe1\xbc\x9a" }, /* Ἒ : up=Ἒ : lo=ἒ */
  { 0xe1bc9b , 0xe1bc9b , 0xe1bc93 , None , u8"\xe1\xbc\x9b" }, /* Ἓ : up=Ἓ : lo=ἓ */
  { 0xe1bc9c , 0xe1bc9c , 0xe1bc94 , None , u8"\xe1\xbc\x9c" }, /* Ἔ : up=Ἔ : lo=ἔ */
  { 0xe1bc9d , 0xe1bc9d , 0xe1bc95 , None , u8"\xe1\xbc\x9d" }, /* Ἕ : up=Ἕ : lo=ἕ */
  { 0xe1bc9e , 0xe1bc9e , 0xe1bc9e , None , u8"\xe1\xbc\x9e" },
  { 0xe1bc9f , 0xe1bc9f , 0xe1bc9f , None , u8"\xe1\xbc\x9f" },
  { 0xe1bca0 , 0xe1bca8 , 0xe1bca0 , None , u8"\xe1\xbc\xa0" }, /* ἠ : up=Ἠ : lo=ἠ */
  { 0xe1bca1 , 0xe1bca9 , 0xe1bca1 , None , u8"\xe1\xbc\xa1" }, /* ἡ : up=Ἡ : lo=ἡ */
  { 0xe1bca2 , 0xe1bcaa , 0xe1bca2 , None , u8"\xe1\xbc\xa2" }, /* ἢ : up=Ἢ : lo=ἢ */
  { 0xe1bca3 , 0xe1bcab , 0xe1bca3 , None , u8"\xe1\xbc\xa3" }, /* ἣ : up=Ἣ : lo=ἣ */
  { 0xe1bca4 , 0xe1bcac , 0xe1bca4 , None , u8"\xe1\xbc\xa4" }, /* ἤ : up=Ἤ : lo=ἤ */
  { 0xe1bca5 , 0xe1bcad , 0xe1bca5 , None , u8"\xe1\xbc\xa5" }, /* ἥ : up=Ἥ : lo=ἥ */
  { 0xe1bca6 , 0xe1bcae , 0xe1bca6 , None , u8"\xe1\xbc\xa6" }, /* ἦ : up=Ἦ : lo=ἦ */
  { 0xe1bca7 , 0xe1bcaf , 0xe1bca7 , None , u8"\xe1\xbc\xa7" }, /* ἧ : up=Ἧ : lo=ἧ */
  { 0xe1bca8 , 0xe1bca8 , 0xe1bca0 , None , u8"\xe1\xbc\xa8" }, /* Ἠ : up=Ἠ : lo=ἠ */
  { 0xe1bca9 , 0xe1bca9 , 0xe1bca1 , None , u8"\xe1\xbc\xa9" }, /* Ἡ : up=Ἡ : lo=ἡ */
  { 0xe1bcaa , 0xe1bcaa , 0xe1bca2 , None , u8"\xe1\xbc\xaa" }, /* Ἢ : up=Ἢ : lo=ἢ */
  { 0xe1bcab , 0xe1bcab , 0xe1bca3 , None , u8"\xe1\xbc\xab" }, /* Ἣ : up=Ἣ : lo=ἣ */
  { 0xe1bcac , 0xe1bcac , 0xe1bca4 , None , u8"\xe1\xbc\xac" }, /* Ἤ : up=Ἤ : lo=ἤ */
  { 0xe1bcad , 0xe1bcad , 0xe1bca5 , None , u8"\xe1\xbc\xad" }, /* Ἥ : up=Ἥ : lo=ἥ */
  { 0xe1bcae , 0xe1bcae , 0xe1bca6 , None , u8"\xe1\xbc\xae" }, /* Ἦ : up=Ἦ : lo=ἦ */
  { 0xe1bcaf , 0xe1bcaf , 0xe1bca7 , None , u8"\xe1\xbc\xaf" }, /* Ἧ : up=Ἧ : lo=ἧ */
  { 0xe1bcb0 , 0xe1bcb8 , 0xe1bcb0 , None , u8"\xe1\xbc\xb0" }, /* ἰ : up=Ἰ : lo=ἰ */
  { 0xe1bcb1 , 0xe1bcb9 , 0xe1bcb1 , None , u8"\xe1\xbc\xb1" }, /* ἱ : up=Ἱ : lo=ἱ */
  { 0xe1bcb2 , 0xe1bcba , 0xe1bcb2 , None , u8"\xe1\xbc\xb2" }, /* ἲ : up=Ἲ : lo=ἲ */
  { 0xe1bcb3 , 0xe1bcbb , 0xe1bcb3 , None , u8"\xe1\xbc\xb3" }, /* ἳ : up=Ἳ : lo=ἳ */
  { 0xe1bcb4 , 0xe1bcbc , 0xe1bcb4 , None , u8"\xe1\xbc\xb4" }, /* ἴ : up=Ἴ : lo=ἴ */
  { 0xe1bcb5 , 0xe1bcbd , 0xe1bcb5 , None , u8"\xe1\xbc\xb5" }, /* ἵ : up=Ἵ : lo=ἵ */
  { 0xe1bcb6 , 0xe1bcbe , 0xe1bcb6 , None , u8"\xe1\xbc\xb6" }, /* ἶ : up=Ἶ : lo=ἶ */
  { 0xe1bcb7 , 0xe1bcbf , 0xe1bcb7 , None , u8"\xe1\xbc\xb7" }, /* ἷ : up=Ἷ : lo=ἷ */
  { 0xe1bcb8 , 0xe1bcb8 , 0xe1bcb0 , None , u8"\xe1\xbc\xb8" }, /* Ἰ : up=Ἰ : lo=ἰ */
  { 0xe1bcb9 , 0xe1bcb9 , 0xe1bcb1 , None , u8"\xe1\xbc\xb9" }, /* Ἱ : up=Ἱ : lo=ἱ */
  { 0xe1bcba , 0xe1bcba , 0xe1bcb2 , None , u8"\xe1\xbc\xba" }, /* Ἲ : up=Ἲ : lo=ἲ */
  { 0xe1bcbb , 0xe1bcbb , 0xe1bcb3 , None , u8"\xe1\xbc\xbb" }, /* Ἳ : up=Ἳ : lo=ἳ */
  { 0xe1bcbc , 0xe1bcbc , 0xe1bcb4 , None , u8"\xe1\xbc\xbc" }, /* Ἴ : up=Ἴ : lo=ἴ */
  { 0xe1bcbd , 0xe1bcbd , 0xe1bcb5 , None , u8"\xe1\xbc\xbd" }, /* Ἵ : up=Ἵ : lo=ἵ */
  { 0xe1bcbe , 0xe1bcbe , 0xe1bcb6 , None , u8"\xe1\xbc\xbe" }, /* Ἶ : up=Ἶ : lo=ἶ */
  { 0xe1bcbf , 0xe1bcbf , 0xe1bcb7 , None , u8"\xe1\xbc\xbf" }, /* Ἷ : up=Ἷ : lo=ἷ */
};

const character charmap_e1_bd[64] = {
  { 0xe1bd80 , 0xe1bd88 , 0xe1bd80 , None , u8"\xe1\xbd\x80" }, /* ὀ : up=Ὀ : lo=ὀ */
  { 0xe1bd81 , 0xe1bd89 , 0xe1bd81 , None , u8"\xe1\xbd\x81" }, /* ὁ : up=Ὁ : lo=ὁ */
  { 0xe1bd82 , 0xe1bd8a , 0xe1bd82 , None , u8"\xe1\xbd\x82" }, /* ὂ : up=Ὂ : lo=ὂ */
  { 0xe1bd83 , 0xe1bd8b , 0xe1bd83 , None , u8"\xe1\xbd\x83" }, /* ὃ : up=Ὃ : lo=ὃ */
  { 0xe1bd84 , 0xe1bd8c , 0xe1bd84 , None , u8"\xe1\xbd\x84" }, /* ὄ : up=Ὄ : lo=ὄ */
  { 0xe1bd85 , 0xe1bd8d , 0xe1bd85 , None , u8"\xe1\xbd\x85" }, /* ὅ : up=Ὅ : lo=ὅ */
  { 0xe1bd86 , 0xe1bd86 , 0xe1bd86 , None , u8"\xe1\xbd\x86" },
  { 0xe1bd87 , 0xe1bd87 , 0xe1bd87 , None , u8"\xe1\xbd\x87" },
  { 0xe1bd88 , 0xe1bd88 , 0xe1bd80 , None , u8"\xe1\xbd\x88" }, /* Ὀ : up=Ὀ : lo=ὀ */
  { 0xe1bd89 , 0xe1bd89 , 0xe1bd81 , None , u8"\xe1\xbd\x89" }, /* Ὁ : up=Ὁ : lo=ὁ */
  { 0xe1bd8a , 0xe1bd8a , 0xe1bd82 , None , u8"\xe1\xbd\x8a" }, /* Ὂ : up=Ὂ : lo=ὂ */
  { 0xe1bd8b , 0xe1bd8b , 0xe1bd83 , None , u8"\xe1\xbd\x8b" }, /* Ὃ : up=Ὃ : lo=ὃ */
  { 0xe1bd8c , 0xe1bd8c , 0xe1bd84 , None , u8"\xe1\xbd\x8c" }, /* Ὄ : up=Ὄ : lo=ὄ */
  { 0xe1bd8d , 0xe1bd8d , 0xe1bd85 , None , u8"\xe1\xbd\x8d" }, /* Ὅ : up=Ὅ : lo=ὅ */
  { 0xe1bd8e , 0xe1bd8e , 0xe1bd8e , None , u8"\xe1\xbd\x8e" },
  { 0xe1bd8f , 0xe1bd8f , 0xe1bd8f , None , u8"\xe1\xbd\x8f" },
  { 0xe1bd90 , 0xe1bd90 , 0xe1bd90 , None , u8"\xe1\xbd\x90" }, /* ὐ : up=ὐ : lo=ὐ */
  { 0xe1bd91 , 0xe1bd99 , 0xe1bd91 , None , u8"\xe1\xbd\x91" }, /* ὑ : up=Ὑ : lo=ὑ */
  { 0xe1bd92 , 0xe1bd92 , 0xe1bd92 , None , u8"\xe1\xbd\x92" }, /* ὒ : up=ὒ : lo=ὒ */
  { 0xe1bd93 , 0xe1bd9b , 0xe1bd93 , None , u8"\xe1\xbd\x93" }, /* ὓ : up=Ὓ : lo=ὓ */
  { 0xe1bd94 , 0xe1bd94 , 0xe1bd94 , None , u8"\xe1\xbd\x94" }, /* ὔ : up=ὔ : lo=ὔ */
  { 0xe1bd95 , 0xe1bd9d , 0xe1bd95 , None , u8"\xe1\xbd\x95" }, /* ὕ : up=Ὕ : lo=ὕ */
  { 0xe1bd96 , 0xe1bd96 , 0xe1bd96 , None , u8"\xe1\xbd\x96" }, /* ὖ : up=ὖ : lo=ὖ */
  { 0xe1bd97 , 0xe1bd9f , 0xe1bd97 , None , u8"\xe1\xbd\x97" }, /* ὗ : up=Ὗ : lo=ὗ */
  { 0xe1bd98 , 0xe1bd98 , 0xe1bd98 , None , u8"\xe1\xbd\x98" },
  { 0xe1bd99 , 0xe1bd99 , 0xe1bd91 , None , u8"\xe1\xbd\x99" }, /* Ὑ : up=Ὑ : lo=ὑ */
  { 0xe1bd9a , 0xe1bd9a , 0xe1bd9a , None , u8"\xe1\xbd\x9a" },
  { 0xe1bd9b , 0xe1bd9b , 0xe1bd93 , None , u8"\xe1\xbd\x9b" }, /* Ὓ : up=Ὓ : lo=ὓ */
  { 0xe1bd9c , 0xe1bd9c , 0xe1bd9c , None , u8"\xe1\xbd\x9c" },
  { 0xe1bd9d , 0xe1bd9d , 0xe1bd95 , None , u8"\xe1\xbd\x9d" }, /* Ὕ : up=Ὕ : lo=ὕ */
  { 0xe1bd9e , 0xe1bd9e , 0xe1bd9e , None , u8"\xe1\xbd\x9e" },
  { 0xe1bd9f , 0xe1bd9f , 0xe1bd97 , None , u8"\xe1\xbd\x9f" }, /* Ὗ : up=Ὗ : lo=ὗ */
  { 0xe1bda0 , 0xe1bda8 , 0xe1bda0 , None , u8"\xe1\xbd\xa0" }, /* ὠ : up=Ὠ : lo=ὠ */
  { 0xe1bda1 , 0xe1bda9 , 0xe1bda1 , None , u8"\xe1\xbd\xa1" }, /* ὡ : up=Ὡ : lo=ὡ */
  { 0xe1bda2 , 0xe1bdaa , 0xe1bda2 , None , u8"\xe1\xbd\xa2" }, /* ὢ : up=Ὢ : lo=ὢ */
  { 0xe1bda3 , 0xe1bdab , 0xe1bda3 , None , u8"\xe1\xbd\xa3" }, /* ὣ : up=Ὣ : lo=ὣ */
  { 0xe1bda4 , 0xe1bdac , 0xe1bda4 , None , u8"\xe1\xbd\xa4" }, /* ὤ : up=Ὤ : lo=ὤ */
  { 0xe1bda5 , 0xe1bdad , 0xe1bda5 , None , u8"\xe1\xbd\xa5" }, /* ὥ : up=Ὥ : lo=ὥ */
  { 0xe1bda6 , 0xe1bdae , 0xe1bda6 , None , u8"\xe1\xbd\xa6" }, /* ὦ : up=Ὦ : lo=ὦ */
  { 0xe1bda7 , 0xe1bdaf , 0xe1bda7 , None , u8"\xe1\xbd\xa7" }, /* ὧ : up=Ὧ : lo=ὧ */
  { 0xe1bda8 , 0xe1bda8 , 0xe1bda0 , None , u8"\xe1\xbd\xa8" }, /* Ὠ : up=Ὠ : lo=ὠ */
  { 0xe1bda9 , 0xe1bda9 , 0xe1bda1 , None , u8"\xe1\xbd\xa9" }, /* Ὡ : up=Ὡ : lo=ὡ */
  { 0xe1bdaa , 0xe1bdaa , 0xe1bda2 , None , u8"\xe1\xbd\xaa" }, /* Ὢ : up=Ὢ : lo=ὢ */
  { 0xe1bdab , 0xe1bdab , 0xe1bda3 , None , u8"\xe1\xbd\xab" }, /* Ὣ : up=Ὣ : lo=ὣ */
  { 0xe1bdac , 0xe1bdac , 0xe1bda4 , None , u8"\xe1\xbd\xac" }, /* Ὤ : up=Ὤ : lo=ὤ */
  { 0xe1bdad , 0xe1bdad , 0xe1bda5 , None , u8"\xe1\xbd\xad" }, /* Ὥ : up=Ὥ : lo=ὥ */
  { 0xe1bdae , 0xe1bdae , 0xe1bda6 , None , u8"\xe1\xbd\xae" }, /* Ὦ : up=Ὦ : lo=ὦ */
  { 0xe1bdaf , 0xe1bdaf , 0xe1bda7 , None , u8"\xe1\xbd\xaf" }, /* Ὧ : up=Ὧ : lo=ὧ */
  { 0xe1bdb0 , 0xe1beba , 0xe1bdb0 , None , u8"\xe1\xbd\xb0" }, /* ὰ : up=Ὰ : lo=ὰ */
  { 0xe1bdb1 , 0xe1bebb , 0xe1bdb1 , None , u8"\xe1\xbd\xb1" }, /* ά : up=Ά : lo=ά */
  { 0xe1bdb2 , 0xe1bf88 , 0xe1bdb2 , None , u8"\xe1\xbd\xb2" }, /* ὲ : up=Ὲ : lo=ὲ */
  { 0xe1bdb3 , 0xe1bf89 , 0xe1bdb3 , None , u8"\xe1\xbd\xb3" }, /* έ : up=Έ : lo=έ */
  { 0xe1bdb4 , 0xe1bf8a , 0xe1bdb4 , None , u8"\xe1\xbd\xb4" }, /* ὴ : up=Ὴ : lo=ὴ */
  { 0xe1bdb5 , 0xe1bf8b , 0xe1bdb5 , None , u8"\xe1\xbd\xb5" }, /* ή : up=Ή : lo=ή */
  { 0xe1bdb6 , 0xe1bf9a , 0xe1bdb6 , None , u8"\xe1\xbd\xb6" }, /* ὶ : up=Ὶ : lo=ὶ */
  { 0xe1bdb7 , 0xe1bf9b , 0xe1bdb7 , None , u8"\xe1\xbd\xb7" }, /* ί : up=Ί : lo=ί */
  { 0xe1bdb8 , 0xe1bfb8 , 0xe1bdb8 , None , u8"\xe1\xbd\xb8" }, /* ὸ : up=Ὸ : lo=ὸ */
  { 0xe1bdb9 , 0xe1bfb9 , 0xe1bdb9 , None , u8"\xe1\xbd\xb9" }, /* ό : up=Ό : lo=ό */
  { 0xe1bdba , 0xe1bfaa , 0xe1bdba , None , u8"\xe1\xbd\xba" }, /* ὺ : up=Ὺ : lo=ὺ */
  { 0xe1bdbb , 0xe1bfab , 0xe1bdbb , None , u8"\xe1\xbd\xbb" }, /* ύ : up=Ύ : lo=ύ */
  { 0xe1bdbc , 0xe1bfba , 0xe1bdbc , None , u8"\xe1\xbd\xbc" }, /* ὼ : up=Ὼ : lo=ὼ */
  { 0xe1bdbd , 0xe1bfbb , 0xe1bdbd , None , u8"\xe1\xbd\xbd" }, /* ώ : up=Ώ : lo=ώ */
  { 0xe1bdbe , 0xe1bdbe , 0xe1bdbe , None , u8"\xe1\xbd\xbe" },
  { 0xe1bdbf , 0xe1bdbf , 0xe1bdbf , None , u8"\xe1\xbd\xbf" },
};

const character charmap_e1_be[64] = {
  { 0xe1be80 , 0xe1be88 , 0xe1be80 , None , u8"\xe1\xbe\x80" }, /* ᾀ : up=ᾈ : lo=ᾀ */
  { 0xe1be81 , 0xe1be89 , 0xe1be81 , None , u8"\xe1\xbe\x81" }, /* ᾁ : up=ᾉ : lo=ᾁ */
  { 0xe1be82 , 0xe1be8a , 0xe1be82 , None , u8"\xe1\xbe\x82" }, /* ᾂ : up=ᾊ : lo=ᾂ */
  { 0xe1be83 , 0xe1be8b , 0xe1be83 , None , u8"\xe1\xbe\x83" }, /* ᾃ : up=ᾋ : lo=ᾃ */
  { 0xe1be84 , 0xe1be8c , 0xe1be84 , None , u8"\xe1\xbe\x84" }, /* ᾄ : up=ᾌ : lo=ᾄ */
  { 0xe1be85 , 0xe1be8d , 0xe1be85 , None , u8"\xe1\xbe\x85" }, /* ᾅ : up=ᾍ : lo=ᾅ */
  { 0xe1be86 , 0xe1be8e , 0xe1be86 , None , u8"\xe1\xbe\x86" }, /* ᾆ : up=ᾎ : lo=ᾆ */
  { 0xe1be87 , 0xe1be8f , 0xe1be87 , None , u8"\xe1\xbe\x87" }, /* ᾇ : up=ᾏ : lo=ᾇ */
  { 0xe1be88 , 0xe1be88 , 0xe1be80 , None , u8"\xe1\xbe\x88" }, /* ᾈ : up=ᾈ : lo=ᾀ */
  { 0xe1be89 , 0xe1be89 , 0xe1be81 , None , u8"\xe1\xbe\x89" }, /* ᾉ : up=ᾉ : lo=ᾁ */
  { 0xe1be8a , 0xe1be8a , 0xe1be82 , None , u8"\xe1\xbe\x8a" }, /* ᾊ : up=ᾊ : lo=ᾂ */
  { 0xe1be8b , 0xe1be8b , 0xe1be83 , None , u8"\xe1\xbe\x8b" }, /* ᾋ : up=ᾋ : lo=ᾃ */
  { 0xe1be8c , 0xe1be8c , 0xe1be84 , None , u8"\xe1\xbe\x8c" }, /* ᾌ : up=ᾌ : lo=ᾄ */
  { 0xe1be8d , 0xe1be8d , 0xe1be85 , None , u8"\xe1\xbe\x8d" }, /* ᾍ : up=ᾍ : lo=ᾅ */
  { 0xe1be8e , 0xe1be8e , 0xe1be86 , None , u8"\xe1\xbe\x8e" }, /* ᾎ : up=ᾎ : lo=ᾆ */
  { 0xe1be8f , 0xe1be8f , 0xe1be87 , None , u8"\xe1\xbe\x8f" }, /* ᾏ : up=ᾏ : lo=ᾇ */
  { 0xe1be90 , 0xe1be98 , 0xe1be90 , None , u8"\xe1\xbe\x90" }, /* ᾐ : up=ᾘ : lo=ᾐ */
  { 0xe1be91 , 0xe1be99 , 0xe1be91 , None , u8"\xe1\xbe\x91" }, /* ᾑ : up=ᾙ : lo=ᾑ */
  { 0xe1be92 , 0xe1be9a , 0xe1be92 , None , u8"\xe1\xbe\x92" }, /* ᾒ : up=ᾚ : lo=ᾒ */
  { 0xe1be93 , 0xe1be9b , 0xe1be93 , None , u8"\xe1\xbe\x93" }, /* ᾓ : up=ᾛ : lo=ᾓ */
  { 0xe1be94 , 0xe1be9c , 0xe1be94 , None , u8"\xe1\xbe\x94" }, /* ᾔ : up=ᾜ : lo=ᾔ */
  { 0xe1be95 , 0xe1be9d , 0xe1be95 , None , u8"\xe1\xbe\x95" }, /* ᾕ : up=ᾝ : lo=ᾕ */
  { 0xe1be96 , 0xe1be9e , 0xe1be96 , None , u8"\xe1\xbe\x96" }, /* ᾖ : up=ᾞ : lo=ᾖ */
  { 0xe1be97 , 0xe1be9f , 0xe1be97 , None , u8"\xe1\xbe\x97" }, /* ᾗ : up=ᾟ : lo=ᾗ */
  { 0xe1be98 , 0xe1be98 , 0xe1be90 , None , u8"\xe1\xbe\x98" }, /* ᾘ : up=ᾘ : lo=ᾐ */
  { 0xe1be99 , 0xe1be99 , 0xe1be91 , None , u8"\xe1\xbe\x99" }, /* ᾙ : up=ᾙ : lo=ᾑ */
  { 0xe1be9a , 0xe1be9a , 0xe1be92 , None , u8"\xe1\xbe\x9a" }, /* ᾚ : up=ᾚ : lo=ᾒ */
  { 0xe1be9b , 0xe1be9b , 0xe1be93 , None , u8"\xe1\xbe\x9b" }, /* ᾛ : up=ᾛ : lo=ᾓ */
  { 0xe1be9c , 0xe1be9c , 0xe1be94 , None , u8"\xe1\xbe\x9c" }, /* ᾜ : up=ᾜ : lo=ᾔ */
  { 0xe1be9d , 0xe1be9d , 0xe1be95 , None , u8"\xe1\xbe\x9d" }, /* ᾝ : up=ᾝ : lo=ᾕ */
  { 0xe1be9e , 0xe1be9e , 0xe1be96 , None , u8"\xe1\xbe\x9e" }, /* ᾞ : up=ᾞ : lo=ᾖ */
  { 0xe1be9f , 0xe1be9f , 0xe1be97 , None , u8"\xe1\xbe\x9f" }, /* ᾟ : up=ᾟ : lo=ᾗ */
  { 0xe1bea0 , 0xe1bea8 , 0xe1bea0 , None , u8"\xe1\xbe\xa0" }, /* ᾠ : up=ᾨ : lo=ᾠ */
  { 0xe1bea1 , 0xe1bea9 , 0xe1bea1 , None , u8"\xe1\xbe\xa1" }, /* ᾡ : up=ᾩ : lo=ᾡ */
  { 0xe1bea2 , 0xe1beaa , 0xe1bea2 , None , u8"\xe1\xbe\xa2" }, /* ᾢ : up=ᾪ : lo=ᾢ */
  { 0xe1bea3 , 0xe1beab , 0xe1bea3 , None , u8"\xe1\xbe\xa3" }, /* ᾣ : up=ᾫ : lo=ᾣ */
  { 0xe1bea4 , 0xe1beac , 0xe1bea4 , None , u8"\xe1\xbe\xa4" }, /* ᾤ : up=ᾬ : lo=ᾤ */
  { 0xe1bea5 , 0xe1bead , 0xe1bea5 , None , u8"\xe1\xbe\xa5" }, /* ᾥ : up=ᾭ : lo=ᾥ */
  { 0xe1bea6 , 0xe1beae , 0xe1bea6 , None , u8"\xe1\xbe\xa6" }, /* ᾦ : up=ᾮ : lo=ᾦ */
  { 0xe1bea7 , 0xe1beaf , 0xe1bea7 , None , u8"\xe1\xbe\xa7" }, /* ᾧ : up=ᾯ : lo=ᾧ */
  { 0xe1bea8 , 0xe1bea8 , 0xe1bea0 , None , u8"\xe1\xbe\xa8" }, /* ᾨ : up=ᾨ : lo=ᾠ */
  { 0xe1bea9 , 0xe1bea9 , 0xe1bea1 , None , u8"\xe1\xbe\xa9" }, /* ᾩ : up=ᾩ : lo=ᾡ */
  { 0xe1beaa , 0xe1beaa , 0xe1bea2 , None , u8"\xe1\xbe\xaa" }, /* ᾪ : up=ᾪ : lo=ᾢ */
  { 0xe1beab , 0xe1beab , 0xe1bea3 , None , u8"\xe1\xbe\xab" }, /* ᾫ : up=ᾫ : lo=ᾣ */
  { 0xe1beac , 0xe1beac , 0xe1bea4 , None , u8"\xe1\xbe\xac" }, /* ᾬ : up=ᾬ : lo=ᾤ */
  { 0xe1bead , 0xe1bead , 0xe1bea5 , None , u8"\xe1\xbe\xad" }, /* ᾭ : up=ᾭ : lo=ᾥ */
  { 0xe1beae , 0xe1beae , 0xe1bea6 , None , u8"\xe1\xbe\xae" }, /* ᾮ : up=ᾮ : lo=ᾦ */
  { 0xe1beaf , 0xe1beaf , 0xe1bea7 , None , u8"\xe1\xbe\xaf" }, /* ᾯ : up=ᾯ : lo=ᾧ */
  { 0xe1beb0 , 0xe1beb8 , 0xe1beb0 , None , u8"\xe1\xbe\xb0" }, /* ᾰ : up=Ᾰ : lo=ᾰ */
  { 0xe1beb1 , 0xe1beb9 , 0xe1beb1 , None , u8"\xe1\xbe\xb1" }, /* ᾱ : up=Ᾱ : lo=ᾱ */
  { 0xe1beb2 , 0xe1beb2 , 0xe1beb2 , None , u8"\xe1\xbe\xb2" }, /* ᾲ : up=ᾲ : lo=ᾲ */
  { 0xe1beb3 , 0xe1bebc , 0xe1beb3 , None , u8"\xe1\xbe\xb3" }, /* ᾳ : up=ᾼ : lo=ᾳ */
  { 0xe1beb4 , 0xe1beb4 , 0xe1beb4 , None , u8"\xe1\xbe\xb4" }, /* ᾴ : up=ᾴ : lo=ᾴ */
  { 0xe1beb5 , 0xe1beb5 , 0xe1beb5 , None , u8"\xe1\xbe\xb5" },
  { 0xe1beb6 , 0xe1beb6 , 0xe1beb6 , None , u8"\xe1\xbe\xb6" }, /* ᾶ : up=ᾶ : lo=ᾶ */
  { 0xe1beb7 , 0xe1beb7 , 0xe1beb7 , None , u8"\xe1\xbe\xb7" }, /* ᾷ : up=ᾷ : lo=ᾷ */
  { 0xe1beb8 , 0xe1beb8 , 0xe1beb0 , None , u8"\xe1\xbe\xb8" }, /* Ᾰ : up=Ᾰ : lo=ᾰ */
  { 0xe1beb9 , 0xe1beb9 , 0xe1beb1 , None , u8"\xe1\xbe\xb9" }, /* Ᾱ : up=Ᾱ : lo=ᾱ */
  { 0xe1beba , 0xe1beba , 0xe1bdb0 , None , u8"\xe1\xbe\xba" }, /* Ὰ : up=Ὰ : lo=ὰ */
  { 0xe1bebb , 0xe1bebb , 0xe1bdb1 , None , u8"\xe1\xbe\xbb" }, /* Ά : up=Ά : lo=ά */
  { 0xe1bebc , 0xe1bebc , 0xe1beb3 , None , u8"\xe1\xbe\xbc" }, /* ᾼ : up=ᾼ : lo=ᾳ */
  { 0xe1bebd , 0xe1bebd , 0xe1bebd , None , u8"\xe1\xbe\xbd" }, /* ᾽ : up=᾽ : lo=᾽ */
  { 0xe1bebe , 0xce99 , 0xe1bebe , None , u8"\xe1\xbe\xbe" }, /* ι : up=Ι : lo=ι */
  { 0xe1bebf , 0xe1bebf , 0xe1bebf , None , u8"\xe1\xbe\xbf" },
};

const character charmap_e1_bf[64] = {
  { 0xe1bf80 , 0xe1bf80 , 0xe1bf80 , None , u8"\xe1\xbf\x80" },
  { 0xe1bf81 , 0xe1bf81 , 0xe1bf81 , None , u8"\xe1\xbf\x81" },
  { 0xe1bf82 , 0xe1bf82 , 0xe1bf82 , None , u8"\xe1\xbf\x82" }, /* ῂ : up=ῂ : lo=ῂ */
  { 0xe1bf83 , 0xe1bf8c , 0xe1bf83 , None , u8"\xe1\xbf\x83" }, /* ῃ : up=ῌ : lo=ῃ */
  { 0xe1bf84 , 0xe1bf84 , 0xe1bf84 , None , u8"\xe1\xbf\x84" }, /* ῄ : up=ῄ : lo=ῄ */
  { 0xe1bf85 , 0xe1bf85 , 0xe1bf85 , None , u8"\xe1\xbf\x85" }, /* ῅ : up=῅ : lo=῅ */
  { 0xe1bf86 , 0xe1bf86 , 0xe1bf86 , None , u8"\xe1\xbf\x86" }, /* ῆ : up=ῆ : lo=ῆ */
  { 0xe1bf87 , 0xe1bf87 , 0xe1bf87 , None , u8"\xe1\xbf\x87" }, /* ῇ : up=ῇ : lo=ῇ */
  { 0xe1bf88 , 0xe1bf88 , 0xe1bdb2 , None , u8"\xe1\xbf\x88" }, /* Ὲ : up=Ὲ : lo=ὲ */
  { 0xe1bf89 , 0xe1bf89 , 0xe1bdb3 , None , u8"\xe1\xbf\x89" }, /* Έ : up=Έ : lo=έ */
  { 0xe1bf8a , 0xe1bf8a , 0xe1bdb4 , None , u8"\xe1\xbf\x8a" }, /* Ὴ : up=Ὴ : lo=ὴ */
  { 0xe1bf8b , 0xe1bf8b , 0xe1bdb5 , None , u8"\xe1\xbf\x8b" }, /* Ή : up=Ή : lo=ή */
  { 0xe1bf8c , 0xe1bf8c , 0xe1bf83 , None , u8"\xe1\xbf\x8c" }, /* ῌ : up=ῌ : lo=ῃ */
  { 0xe1bf8d , 0xe1bf8d , 0xe1bf8d , None , u8"\xe1\xbf\x8d" },
  { 0xe1bf8e , 0xe1bf8e , 0xe1bf8e , None , u8"\xe1\xbf\x8e" },
  { 0xe1bf8f , 0xe1bf8f , 0xe1bf8f , None , u8"\xe1\xbf\x8f" },
  { 0xe1bf90 , 0xe1bf98 , 0xe1bf90 , None , u8"\xe1\xbf\x90" }, /* ῐ : up=Ῐ : lo=ῐ */
  { 0xe1bf91 , 0xe1bf99 , 0xe1bf91 , None , u8"\xe1\xbf\x91" }, /* ῑ : up=Ῑ : lo=ῑ */
  { 0xe1bf92 , 0xe1bf92 , 0xe1bf92 , None , u8"\xe1\xbf\x92" }, /* ῒ : up=ῒ : lo=ῒ */
  { 0xe1bf93 , 0xe1bf93 , 0xe1bf93 , None , u8"\xe1\xbf\x93" }, /* ΐ : up=ΐ : lo=ΐ */
  { 0xe1bf94 , 0xe1bf94 , 0xe1bf94 , None , u8"\xe1\xbf\x94" },
  { 0xe1bf95 , 0xe1bf95 , 0xe1bf95 , None , u8"\xe1\xbf\x95" },
  { 0xe1bf96 , 0xe1bf96 , 0xe1bf96 , None , u8"\xe1\xbf\x96" }, /* ῖ : up=ῖ : lo=ῖ */
  { 0xe1bf97 , 0xe1bf97 , 0xe1bf97 , None , u8"\xe1\xbf\x97" }, /* ῗ : up=ῗ : lo=ῗ */
  { 0xe1bf98 , 0xe1bf98 , 0xe1bf90 , None , u8"\xe1\xbf\x98" }, /* Ῐ : up=Ῐ : lo=ῐ */
  { 0xe1bf99 , 0xe1bf99 , 0xe1bf91 , None , u8"\xe1\xbf\x99" }, /* Ῑ : up=Ῑ : lo=ῑ */
  { 0xe1bf9a , 0xe1bf9a , 0xe1bdb6 , None , u8"\xe1\xbf\x9a" }, /* Ὶ : up=Ὶ : lo=ὶ */
  { 0xe1bf9b , 0xe1bf9b , 0xe1bdb7 , None , u8"\xe1\xbf\x9b" }, /* Ί : up=Ί : lo=ί */
  { 0xe1bf9c , 0xe1bf9c , 0xe1bf9c , None , u8"\xe1\xbf\x9c" },
  { 0xe1bf9d , 0xe1bf9d , 0xe1bf9d , None , u8"\xe1\xbf\x9d" }, /* ῝ : up=῝ : lo=῝ */
  { 0xe1bf9e , 0xe1bf9e , 0xe1bf9e , None , u8"\xe1\xbf\x9e" }, /* ῞ : up=῞ : lo=῞ */
  { 0xe1bf9f , 0xe1bf9f , 0xe1bf9f , None , u8"\xe1\xbf\x9f" }, /* ῟ : up=῟ : lo=῟ */
  { 0xe1bfa0 , 0xe1bfa8 , 0xe1bfa0 , None , u8"\xe1\xbf\xa0" }, /* ῠ : up=Ῠ : lo=ῠ */
  { 0xe1bfa1 , 0xe1bfa9 , 0xe1bfa1 , None , u8"\xe1\xbf\xa1" }, /* ῡ : up=Ῡ : lo=ῡ */
  { 0xe1bfa2 , 0xe1bfa2 , 0xe1bfa2 , None , u8"\xe1\xbf\xa2" }, /* ῢ : up=ῢ : lo=ῢ */
  { 0xe1bfa3 , 0xe1bfa3 , 0xe1bfa3 , None , u8"\xe1\xbf\xa3" }, /* ΰ : up=ΰ : lo=ΰ */
  { 0xe1bfa4 , 0xe1bfa4 , 0xe1bfa4 , None , u8"\xe1\xbf\xa4" }, /* ῤ : up=ῤ : lo=ῤ */
  { 0xe1bfa5 , 0xe1bfac , 0xe1bfa5 , None , u8"\xe1\xbf\xa5" }, /* ῥ : up=Ῥ : lo=ῥ */
  { 0xe1bfa6 , 0xe1bfa6 , 0xe1bfa6 , None , u8"\xe1\xbf\xa6" }, /* ῦ : up=ῦ : lo=ῦ */
  { 0xe1bfa7 , 0xe1bfa7 , 0xe1bfa7 , None , u8"\xe1\xbf\xa7" }, /* ῧ : up=ῧ : lo=ῧ */
  { 0xe1bfa8 , 0xe1bfa8 , 0xe1bfa0 , None , u8"\xe1\xbf\xa8" }, /* Ῠ : up=Ῠ : lo=ῠ */
  { 0xe1bfa9 , 0xe1bfa9 , 0xe1bfa1 , None , u8"\xe1\xbf\xa9" }, /* Ῡ : up=Ῡ : lo=ῡ */
  { 0xe1bfaa , 0xe1bfaa , 0xe1bdba , None , u8"\xe1\xbf\xaa" }, /* Ὺ : up=Ὺ : lo=ὺ */
  { 0xe1bfab , 0xe1bfab , 0xe1bdbb , None , u8"\xe1\xbf\xab" }, /* Ύ : up=Ύ : lo=ύ */
  { 0xe1bfac , 0xe1bfac , 0xe1bfa5 , None , u8"\xe1\xbf\xac" }, /* Ῥ : up=Ῥ : lo=ῥ */
  { 0xe1bfad , 0xe1bfad , 0xe1bfad , None , u8"\xe1\xbf\xad" },
  { 0xe1bfae , 0xe1bfae , 0xe1bfae , None , u8"\xe1\xbf\xae" },
  { 0xe1bfaf , 0xe1bfaf , 0xe1bfaf , None , u8"\xe1\xbf\xaf" },
  { 0xe1bfb0 , 0xe1bfb0 , 0xe1bfb0 , None , u8"\xe1\xbf\xb0" },
  { 0xe1bfb1 , 0xe1bfb1 , 0xe1bfb1 , None , u8"\xe1\xbf\xb1" },
  { 0xe1bfb2 , 0xe1bfb2 , 0xe1bfb2 , None , u8"\xe1\xbf\xb2" }, /* ῲ : up=ῲ : lo=ῲ */
  { 0xe1bfb3 , 0xe1bfbc , 0xe1bfb3 , None , u8"\xe1\xbf\xb3" }, /* ῳ : up=ῼ : lo=ῳ */
  { 0xe1bfb4 , 0xe1bfb4 , 0xe1bfb4 , None , u8"\xe1\xbf\xb4" }, /* ῴ : up=ῴ : lo=ῴ */
  { 0xe1bfb5 , 0xe1bfb5 , 0xe1bfb5 , None , u8"\xe1\xbf\xb5" },
  { 0xe1bfb6 , 0xe1bfb6 , 0xe1bfb6 , None , u8"\xe1\xbf\xb6" }, /* ῶ : up=ῶ : lo=ῶ */
  { 0xe1bfb7 , 0xe1bfb7 , 0xe1bfb7 , None , u8"\xe1\xbf\xb7" }, /* ῷ : up=ῷ : lo=ῷ */
  { 0xe1bfb8 , 0xe1bfb8 , 0xe1bdb8 , None , u8"\xe1\xbf\xb8" }, /* Ὸ : up=Ὸ : lo=ὸ */
  { 0xe1bfb9 , 0xe1bfb9 , 0xe1bdb9 , None , u8"\xe1\xbf\xb9" }, /* Ό : up=Ό : lo=ό */
  { 0xe1bfba , 0xe1bfba , 0xe1bdbc , None , u8"\xe1\xbf\xba" }, /* Ὼ : up=Ὼ : lo=ὼ */
  { 0xe1bfbb , 0xe1bfbb , 0xe1bdbd , None , u8"\xe1\xbf\xbb" }, /* Ώ : up=Ώ : lo=ώ */
  { 0xe1bfbc , 0xe1bfbc , 0xe1bfb3 , None , u8"\xe1\xbf\xbc" }, /* ῼ : up=ῼ : lo=ῳ */
  { 0xe1bfbd , 0xe1bfbd , 0xe1bfbd , None , u8"\xe1\xbf\xbd" },
  { 0xe1bfbe , 0xe1bfbe , 0xe1bfbe , None , u8"\xe1\xbf\xbe" },
  { 0xe1bfbf , 0xe1bfbf , 0xe1bfbf , None , u8"\xe1\xbf\xbf" },
};

const character* pagemap_24_e2[64] = {
  /* 80 */
  charmap_e2_80, charmap_e2_81, charmap_e2_82, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, charmap_e2_b4, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

const character charmap_e2_80[64] = {
  { 0xe28080 , 0xe28080 , 0xe28080 , None , u8"\xe2\x80\x80" },
  { 0xe28081 , 0xe28081 , 0xe28081 , None , u8"\xe2\x80\x81" },
  { 0xe28082 , 0xe28082 , 0xe28082 , None , u8"\xe2\x80\x82" },
  { 0xe28083 , 0xe28083 , 0xe28083 , None , u8"\xe2\x80\x83" },
  { 0xe28084 , 0xe28084 , 0xe28084 , None , u8"\xe2\x80\x84" },
  { 0xe28085 , 0xe28085 , 0xe28085 , None , u8"\xe2\x80\x85" },
  { 0xe28086 , 0xe28086 , 0xe28086 , None , u8"\xe2\x80\x86" },
  { 0xe28087 , 0xe28087 , 0xe28087 , IsSpace | IsBreaker , u8"\xe2\x80\x87" }, /* Figure Space */
  { 0xe28088 , 0xe28088 , 0xe28088 , None , u8"\xe2\x80\x88" },
  { 0xe28089 , 0xe28089 , 0xe28089 , None , u8"\xe2\x80\x89" },
  { 0xe2808a , 0xe2808a , 0xe2808a , None , u8"\xe2\x80\x8a" },
  { 0xe2808b , 0xe2808b , 0xe2808b , None , u8"\xe2\x80\x8b" },
  { 0xe2808c , 0xe2808c , 0xe2808c , None , u8"\xe2\x80\x8c" },
  { 0xe2808d , 0xe2808d , 0xe2808d , None , u8"\xe2\x80\x8d" },
  { 0xe2808e , 0xe2808e , 0xe2808e , None , u8"\xe2\x80\x8e" },
  { 0xe2808f , 0xe2808f , 0xe2808f , None , u8"\xe2\x80\x8f" },
  { 0xe28090 , 0xe28090 , 0xe28090 , IsPunctuation , "-" }, /* ‐ : up=‐ : lo=‐ */
  { 0xe28091 , 0xe28091 , 0xe28091 , IsPunctuation , "-" }, /* ‑ : up=‑ : lo=‑ */
  { 0xe28092 , 0xe28092 , 0xe28092 , IsPunctuation , "--" }, /* ‒ : up=‒ : lo=‒ */
  { 0xe28093 , 0xe28093 , 0xe28093 , IsPunctuation , "--" }, /* – : up=– : lo=– */
  { 0xe28094 , 0xe28094 , 0xe28094 , IsPunctuation , "--" }, /* — : up=— : lo=— */
  { 0xe28095 , 0xe28095 , 0xe28095 , IsPunctuation , "--" }, /* ― : up=― : lo=― */
  { 0xe28096 , 0xe28096 , 0xe28096 , None , u8"\xe2\x80\x96" }, /* ‖ : up=‖ : lo=‖ */
  { 0xe28097 , 0xe28097 , 0xe28097 , None , u8"\xe2\x80\x97" }, /* ‗ : up=‗ : lo=‗ */
  { 0xe28098 , 0xe28098 , 0xe28098 , IsPunctuation , "'" }, /* ‘ : up=‘ : lo=‘ */
  { 0xe28099 , 0xe28099 , 0xe28099 , IsPunctuation , "'" }, /* ’ : up=’ : lo=’ */
  { 0xe2809a , 0xe2809a , 0xe2809a , IsPunctuation , "'" }, /* ‚ : up=‚ : lo=‚ */
  { 0xe2809b , 0xe2809b , 0xe2809b , IsPunctuation , "'" }, /* ‛ : up=‛ : lo=‛ */
  { 0xe2809c , 0xe2809c , 0xe2809c , IsPunctuation , "\"" }, /* “ : up=“ : lo=“ */
  { 0xe2809d , 0xe2809d , 0xe2809d , IsPunctuation , "\"" }, /* ” : up=” : lo=” */
  { 0xe2809e , 0xe2809e , 0xe2809e , IsPunctuation , "\"" }, /* „ : up=„ : lo=„ */
  { 0xe2809f , 0xe2809f , 0xe2809f , IsPunctuation , "\"" }, /* ‟ : up=‟ : lo=‟ */
  { 0xe280a0 , 0xe280a0 , 0xe280a0 , None , u8"\xe2\x80\xa0" }, /* † : up=† : lo=† */
  { 0xe280a1 , 0xe280a1 , 0xe280a1 , None , u8"\xe2\x80\xa1" }, /* ‡ : up=‡ : lo=‡ */
  { 0xe280a2 , 0xe280a2 , 0xe280a2 , IsPunctuation , "o" }, /* • : up=• : lo=• */
  { 0xe280a3 , 0xe280a3 , 0xe280a3 , None , u8"\xe2\x80\xa3" }, /* ‣ : up=‣ : lo=‣ */
  { 0xe280a4 , 0xe280a4 , 0xe280a4 , IsPunctuation , "." }, /* ․ : up=․ : lo=․ */
  { 0xe280a5 , 0xe280a5 , 0xe280a5 , IsPunctuation , ".." }, /* ‥ : up=‥ : lo=‥ */
  { 0xe280a6 , 0xe280a6 , 0xe280a6 , IsPunctuation , "..." }, /* … : up=… : lo=… */
  { 0xe280a7 , 0xe280a7 , 0xe280a7 , IsPunctuation , u8"\xe2\x80\xa7" }, /* ‧ : up=‧ : lo=‧ */
  { 0xe280a8 , 0xe280a8 , 0xe280a8 , None , u8"\xe2\x80\xa8" },
  { 0xe280a9 , 0xe280a9 , 0xe280a9 , None , u8"\xe2\x80\xa9" },
  { 0xe280aa , 0xe280aa , 0xe280aa , None , u8"\xe2\x80\xaa" },
  { 0xe280ab , 0xe280ab , 0xe280ab , None , u8"\xe2\x80\xab" },
  { 0xe280ac , 0xe280ac , 0xe280ac , None , u8"\xe2\x80\xac" },
  { 0xe280ad , 0xe280ad , 0xe280ad , None , u8"\xe2\x80\xad" },
  { 0xe280ae , 0xe280ae , 0xe280ae , None , u8"\xe2\x80\xae" },
  { 0xe280af , 0xe280af , 0xe280af , IsSpace , u8"\xe2\x80\xaf" }, /* Narrow No-Break Space (NNBSP) */
  { 0xe280b0 , 0xe280b0 , 0xe280b0 , None , u8"\xe2\x80\xb0" }, /* ‰ : up=‰ : lo=‰ */
  { 0xe280b1 , 0xe280b1 , 0xe280b1 , None , u8"\xe2\x80\xb1" }, /* ‱ : up=‱ : lo=‱ */
  { 0xe280b2 , 0xe280b2 , 0xe280b2 , None , u8"\xe2\x80\xb2" }, /* ′ : up=′ : lo=′ */
  { 0xe280b3 , 0xe280b3 , 0xe280b3 , None , u8"\xe2\x80\xb3" }, /* ″ : up=″ : lo=″ */
  { 0xe280b4 , 0xe280b4 , 0xe280b4 , None , u8"\xe2\x80\xb4" }, /* ‴ : up=‴ : lo=‴ */
  { 0xe280b5 , 0xe280b5 , 0xe280b5 , None , u8"\xe2\x80\xb5" }, /* ‵ : up=‵ : lo=‵ */
  { 0xe280b6 , 0xe280b6 , 0xe280b6 , None , u8"\xe2\x80\xb6" }, /* ‶ : up=‶ : lo=‶ */
  { 0xe280b7 , 0xe280b7 , 0xe280b7 , None , u8"\xe2\x80\xb7" }, /* ‷ : up=‷ : lo=‷ */
  { 0xe280b8 , 0xe280b8 , 0xe280b8 , None , u8"\xe2\x80\xb8" }, /* ‸ : up=‸ : lo=‸ */
  { 0xe280b9 , 0xe280b9 , 0xe280b9 , None , u8"\xe2\x80\xb9" }, /* ‹ : up=‹ : lo=‹ */
  { 0xe280ba , 0xe280ba , 0xe280ba , None , u8"\xe2\x80\xba" }, /* › : up=› : lo=› */
  { 0xe280bb , 0xe280bb , 0xe280bb , None , u8"\xe2\x80\xbb" }, /* ※ : up=※ : lo=※ */
  { 0xe280bc , 0xe280bc , 0xe280bc , None , u8"\xe2\x80\xbc" }, /* ‼ : up=‼ : lo=‼ */
  { 0xe280bd , 0xe280bd , 0xe280bd , None , u8"\xe2\x80\xbd" }, /* ‽ : up=‽ : lo=‽ */
  { 0xe280be , 0xe280be , 0xe280be , None , u8"\xe2\x80\xbe" }, /* ‾ : up=‾ : lo=‾ */
  { 0xe280bf , 0xe280bf , 0xe280bf , None , u8"\xe2\x80\xbf" }, /* ‿ : up=‿ : lo=‿ */
};

const character charmap_e2_81[64] = {
  { 0xe28180 , 0xe28180 , 0xe28180 , None , u8"\xe2\x81\x80" }, /* ⁀ : up=⁀ : lo=⁀ */
  { 0xe28181 , 0xe28181 , 0xe28181 , None , u8"\xe2\x81\x81" }, /* ⁁ : up=⁁ : lo=⁁ */
  { 0xe28182 , 0xe28182 , 0xe28182 , None , u8"\xe2\x81\x82" }, /* ⁂ : up=⁂ : lo=⁂ */
  { 0xe28183 , 0xe28183 , 0xe28183 , None , u8"\xe2\x81\x83" }, /* ⁃ : up=⁃ : lo=⁃ */
  { 0xe28184 , 0xe28184 , 0xe28184 , None , "/" }, /* ⁄ : up=⁄ : lo=⁄ */
  { 0xe28185 , 0xe28185 , 0xe28185 , None , u8"\xe2\x81\x85" }, /* ⁅ : up=⁅ : lo=⁅ */
  { 0xe28186 , 0xe28186 , 0xe28186 , None , u8"\xe2\x81\x86" }, /* ⁆ : up=⁆ : lo=⁆ */
  { 0xe28187 , 0xe28187 , 0xe28187 , None , "??" }, /* ⁇ : up=⁇ : lo=⁇ */
  { 0xe28188 , 0xe28188 , 0xe28188 , None , "?!" }, /* ⁈ : up=⁈ : lo=⁈ */
  { 0xe28189 , 0xe28189 , 0xe28189 , None , "!?" }, /* ⁉ : up=⁉ : lo=⁉ */
  { 0xe2818a , 0xe2818a , 0xe2818a , None , "&" }, /* ⁊ : up=⁊ : lo=⁊ */
  { 0xe2818b , 0xe2818b , 0xe2818b , None , u8"\xe2\x81\x8b" }, /* ⁋ : up=⁋ : lo=⁋ */
  { 0xe2818c , 0xe2818c , 0xe2818c , None , u8"\xe2\x81\x8c" }, /* ⁌ : up=⁌ : lo=⁌ */
  { 0xe2818d , 0xe2818d , 0xe2818d , None , u8"\xe2\x81\x8d" }, /* ⁍ : up=⁍ : lo=⁍ */
  { 0xe2818e , 0xe2818e , 0xe2818e , None , u8"\xe2\x81\x8e" }, /* ⁎ : up=⁎ : lo=⁎ */
  { 0xe2818f , 0xe2818f , 0xe2818f , None , u8"\xe2\x81\x8f" }, /* ⁏ : up=⁏ : lo=⁏ */
  { 0xe28190 , 0xe28190 , 0xe28190 , None , u8"\xe2\x81\x90" }, /* ⁐ : up=⁐ : lo=⁐ */
  { 0xe28191 , 0xe28191 , 0xe28191 , None , u8"\xe2\x81\x91" }, /* ⁑ : up=⁑ : lo=⁑ */
  { 0xe28192 , 0xe28192 , 0xe28192 , None , u8"\xe2\x81\x92" }, /* ⁒ : up=⁒ : lo=⁒ */
  { 0xe28193 , 0xe28193 , 0xe28193 , None , u8"\xe2\x81\x93" }, /* ⁓ : up=⁓ : lo=⁓ */
  { 0xe28194 , 0xe28194 , 0xe28194 , None , u8"\xe2\x81\x94" }, /* ⁔ : up=⁔ : lo=⁔ */
  { 0xe28195 , 0xe28195 , 0xe28195 , None , u8"\xe2\x81\x95" }, /* ⁕ : up=⁕ : lo=⁕ */
  { 0xe28196 , 0xe28196 , 0xe28196 , None , u8"\xe2\x81\x96" }, /* ⁖ : up=⁖ : lo=⁖ */
  { 0xe28197 , 0xe28197 , 0xe28197 , None , u8"\xe2\x81\x97" }, /* ⁗ : up=⁗ : lo=⁗ */
  { 0xe28198 , 0xe28198 , 0xe28198 , None , u8"\xe2\x81\x98" }, /* ⁘ : up=⁘ : lo=⁘ */
  { 0xe28199 , 0xe28199 , 0xe28199 , None , u8"\xe2\x81\x99" }, /* ⁙ : up=⁙ : lo=⁙ */
  { 0xe2819a , 0xe2819a , 0xe2819a , None , u8"\xe2\x81\x9a" }, /* ⁚ : up=⁚ : lo=⁚ */
  { 0xe2819b , 0xe2819b , 0xe2819b , None , u8"\xe2\x81\x9b" }, /* ⁛ : up=⁛ : lo=⁛ */
  { 0xe2819c , 0xe2819c , 0xe2819c , None , u8"\xe2\x81\x9c" }, /* ⁜ : up=⁜ : lo=⁜ */
  { 0xe2819d , 0xe2819d , 0xe2819d , None , u8"\xe2\x81\x9d" }, /* ⁝ : up=⁝ : lo=⁝ */
  { 0xe2819e , 0xe2819e , 0xe2819e , None , u8"\xe2\x81\x9e" }, /* ⁞ : up=⁞ : lo=⁞ */
  { 0xe2819f , 0xe2819f , 0xe2819f , IsSpace , " " }, /* (ESP MM) */
  { 0xe281a0 , 0xe281a0 , 0xe281a0 , None , "" }, /* hidden */
  { 0xe281a1 , 0xe281a1 , 0xe281a1 , None , "" }, /* hidden */
  { 0xe281a2 , 0xe281a2 , 0xe281a2 , None , "" }, /* hidden */
  { 0xe281a3 , 0xe281a3 , 0xe281a3 , None , "" }, /* hidden */
  { 0xe281a4 , 0xe281a4 , 0xe281a4 , None , "" }, /* hidden */
  { 0xe281a5 , 0xe281a5 , 0xe281a5 , None , u8"\xe2\x81\xa5" },
  { 0xe281a6 , 0xe281a6 , 0xe281a6 , None , u8"\xe2\x81\xa6" },
  { 0xe281a7 , 0xe281a7 , 0xe281a7 , None , u8"\xe2\x81\xa7" },
  { 0xe281a8 , 0xe281a8 , 0xe281a8 , None , u8"\xe2\x81\xa8" },
  { 0xe281a9 , 0xe281a9 , 0xe281a9 , None , u8"\xe2\x81\xa9" },
  { 0xe281aa , 0xe281aa , 0xe281aa , None , u8"\xe2\x81\xaa" },
  { 0xe281ab , 0xe281ab , 0xe281ab , None , u8"\xe2\x81\xab" },
  { 0xe281ac , 0xe281ac , 0xe281ac , None , u8"\xe2\x81\xac" },
  { 0xe281ad , 0xe281ad , 0xe281ad , None , u8"\xe2\x81\xad" },
  { 0xe281ae , 0xe281ae , 0xe281ae , None , u8"\xe2\x81\xae" },
  { 0xe281af , 0xe281af , 0xe281af , None , u8"\xe2\x81\xaf" },
  { 0xe281b0 , 0xe281b0 , 0xe281b0 , None , "0" }, /* ⁰ : up=⁰ : lo=⁰ */
  { 0xe281b1 , 0xe281b1 , 0xe281b1 , None , "i" }, /* ⁱ : up=ⁱ : lo=ⁱ */
  { 0xe281b2 , 0xe281b2 , 0xe281b2 , None , u8"\xe2\x81\xb2" },
  { 0xe281b3 , 0xe281b3 , 0xe281b3 , None , u8"\xe2\x81\xb3" },
  { 0xe281b4 , 0xe281b4 , 0xe281b4 , None , "4" }, /* ⁴ : up=⁴ : lo=⁴ */
  { 0xe281b5 , 0xe281b5 , 0xe281b5 , None , "5" }, /* ⁵ : up=⁵ : lo=⁵ */
  { 0xe281b6 , 0xe281b6 , 0xe281b6 , None , "6" }, /* ⁶ : up=⁶ : lo=⁶ */
  { 0xe281b7 , 0xe281b7 , 0xe281b7 , None , "7" }, /* ⁷ : up=⁷ : lo=⁷ */
  { 0xe281b8 , 0xe281b8 , 0xe281b8 , None , "8" }, /* ⁸ : up=⁸ : lo=⁸ */
  { 0xe281b9 , 0xe281b9 , 0xe281b9 , None , "9" }, /* ⁹ : up=⁹ : lo=⁹ */
  { 0xe281ba , 0xe281ba , 0xe281ba , None , "+" }, /* ⁺ : up=⁺ : lo=⁺ */
  { 0xe281bb , 0xe281bb , 0xe281bb , None , "-" }, /* ⁻ : up=⁻ : lo=⁻ */
  { 0xe281bc , 0xe281bc , 0xe281bc , None , "=" }, /* ⁼ : up=⁼ : lo=⁼ */
  { 0xe281bd , 0xe281bd , 0xe281bd , None , "(" }, /* ⁽ : up=⁽ : lo=⁽ */
  { 0xe281be , 0xe281be , 0xe281be , None , ")" }, /* ⁾ : up=⁾ : lo=⁾ */
  { 0xe281bf , 0xe281bf , 0xe281bf , None , "n" }, /* ⁿ : up=ⁿ : lo=ⁿ */
};

const character charmap_e2_82[64] = {
  { 0xe28280 , 0xe28280 , 0xe28280 , None , "0" }, /* ₀ : up=₀ : lo=₀ */
  { 0xe28281 , 0xe28281 , 0xe28281 , None , "1" }, /* ₁ : up=₁ : lo=₁ */
  { 0xe28282 , 0xe28282 , 0xe28282 , None , "2" }, /* ₂ : up=₂ : lo=₂ */
  { 0xe28283 , 0xe28283 , 0xe28283 , None , "3" }, /* ₃ : up=₃ : lo=₃ */
  { 0xe28284 , 0xe28284 , 0xe28284 , None , "4" }, /* ₄ : up=₄ : lo=₄ */
  { 0xe28285 , 0xe28285 , 0xe28285 , None , "5" }, /* ₅ : up=₅ : lo=₅ */
  { 0xe28286 , 0xe28286 , 0xe28286 , None , "6" }, /* ₆ : up=₆ : lo=₆ */
  { 0xe28287 , 0xe28287 , 0xe28287 , None , "7" }, /* ₇ : up=₇ : lo=₇ */
  { 0xe28288 , 0xe28288 , 0xe28288 , None , "8" }, /* ₈ : up=₈ : lo=₈ */
  { 0xe28289 , 0xe28289 , 0xe28289 , None , "9" }, /* ₉ : up=₉ : lo=₉ */
  { 0xe2828a , 0xe2828a , 0xe2828a , None , "+" }, /* ₊ : up=₊ : lo=₊ */
  { 0xe2828b , 0xe2828b , 0xe2828b , None , "-" }, /* ₋ : up=₋ : lo=₋ */
  { 0xe2828c , 0xe2828c , 0xe2828c , None , "=" }, /* ₌ : up=₌ : lo=₌ */
  { 0xe2828d , 0xe2828d , 0xe2828d , None , "(" }, /* ₍ : up=₍ : lo=₍ */
  { 0xe2828e , 0xe2828e , 0xe2828e , None , ")" }, /* ₎ : up=₎ : lo=₎ */
  { 0xe2828f , 0xe2828f , 0xe2828f , None , u8"\xe2\x82\x8f" }, /* ₏ : up=₏ : lo=₏ */
  { 0xe28290 , 0xe28290 , 0xe28290 , None , "a" }, /* ₐ : up=ₐ : lo=ₐ */
  { 0xe28291 , 0xe28291 , 0xe28291 , None , "e" }, /* ₑ : up=ₑ : lo=ₑ */
  { 0xe28292 , 0xe28292 , 0xe28292 , None , "o" }, /* ₒ : up=ₒ : lo=ₒ */
  { 0xe28293 , 0xe28293 , 0xe28293 , None , "x" }, /* ₓ : up=ₓ : lo=ₓ */
  { 0xe28294 , 0xe28294 , 0xe28294 , None , u8"\xe2\x82\x94" }, /* ₔ : up=ₔ : lo=ₔ */
  { 0xe28295 , 0xe28295 , 0xe28295 , None , "h" }, /* ₕ : up=ₕ : lo=ₕ */
  { 0xe28296 , 0xe28296 , 0xe28296 , None , "k" }, /* ₖ : up=ₖ : lo=ₖ */
  { 0xe28297 , 0xe28297 , 0xe28297 , None , "l" }, /* ₗ : up=ₗ : lo=ₗ */
  { 0xe28298 , 0xe28298 , 0xe28298 , None , "m" }, /* ₘ : up=ₘ : lo=ₘ */
  { 0xe28299 , 0xe28299 , 0xe28299 , None , "n" }, /* ₙ : up=ₙ : lo=ₙ */
  { 0xe2829a , 0xe2829a , 0xe2829a , None , "p" }, /* ₚ : up=ₚ : lo=ₚ */
  { 0xe2829b , 0xe2829b , 0xe2829b , None , "s" }, /* ₛ : up=ₛ : lo=ₛ */
  { 0xe2829c , 0xe2829c , 0xe2829c , None , "t" }, /* ₜ : up=ₜ : lo=ₜ */
  { 0xe2829d , 0xe2829d , 0xe2829d , None , u8"\xe2\x82\x9d" },
  { 0xe2829e , 0xe2829e , 0xe2829e , None , u8"\xe2\x82\x9e" },
  { 0xe2829f , 0xe2829f , 0xe2829f , None , u8"\xe2\x82\x9f" },
  { 0xe282a0 , 0xe282a0 , 0xe282a0 , None , "CE" }, /* ₠ : up=₠ : lo=₠ */
  { 0xe282a1 , 0xe282a1 , 0xe282a1 , None , "C=" }, /* ₡ : up=₡ : lo=₡ */
  { 0xe282a2 , 0xe282a2 , 0xe282a2 , None , "Cr" }, /* ₢ : up=₢ : lo=₢ */
  { 0xe282a3 , 0xe282a3 , 0xe282a3 , None , "Fr." }, /* ₣ : up=₣ : lo=₣ */
  { 0xe282a4 , 0xe282a4 , 0xe282a4 , None , "L." }, /* ₤ : up=₤ : lo=₤ */
  { 0xe282a5 , 0xe282a5 , 0xe282a5 , None , u8"\xe2\x82\xa5" }, /* ₥ : up=₥ : lo=₥ */
  { 0xe282a6 , 0xe282a6 , 0xe282a6 , None , u8"\xe2\x82\xa6" }, /* ₦ : up=₦ : lo=₦ */
  { 0xe282a7 , 0xe282a7 , 0xe282a7 , None , "Pts" }, /* ₧ : up=₧ : lo=₧ */
  { 0xe282a8 , 0xe282a8 , 0xe282a8 , None , "Rs" }, /* ₨ : up=₨ : lo=₨ */
  { 0xe282a9 , 0xe282a9 , 0xe282a9 , None , "KRW" }, /* ₩ : up=₩ : lo=₩ */
  { 0xe282aa , 0xe282aa , 0xe282aa , None , "ILS" }, /* ₪ : up=₪ : lo=₪ */
  { 0xe282ab , 0xe282ab , 0xe282ab , None , "Dong" }, /* ₫ : up=₫ : lo=₫ */
  { 0xe282ac , 0xe282ac , 0xe282ac , None , "EUR" }, /* € : up=€ : lo=€ */
  { 0xe282ad , 0xe282ad , 0xe282ad , None , u8"\xe2\x82\xad" }, /* ₭ : up=₭ : lo=₭ */
  { 0xe282ae , 0xe282ae , 0xe282ae , None , u8"\xe2\x82\xae" }, /* ₮ : up=₮ : lo=₮ */
  { 0xe282af , 0xe282af , 0xe282af , None , "GRD" }, /* ₯ : up=₯ : lo=₯ */
  { 0xe282b0 , 0xe282b0 , 0xe282b0 , None , u8"\xe2\x82\xb0" }, /* ₰ : up=₰ : lo=₰ */
  { 0xe282b1 , 0xe282b1 , 0xe282b1 , None , "PHP" }, /* ₱ : up=₱ : lo=₱ */
  { 0xe282b2 , 0xe282b2 , 0xe282b2 , None , u8"\xe2\x82\xb2" }, /* ₲ : up=₲ : lo=₲ */
  { 0xe282b3 , 0xe282b3 , 0xe282b3 , None , u8"\xe2\x82\xb3" }, /* ₳ : up=₳ : lo=₳ */
  { 0xe282b4 , 0xe282b4 , 0xe282b4 , None , "UAH" }, /* ₴ : up=₴ : lo=₴ */
  { 0xe282b5 , 0xe282b5 , 0xe282b5 , None , u8"\xe2\x82\xb5" }, /* ₵ : up=₵ : lo=₵ */
  { 0xe282b6 , 0xe282b6 , 0xe282b6 , None , u8"\xe2\x82\xb6" }, /* ₶ : up=₶ : lo=₶ */
  { 0xe282b7 , 0xe282b7 , 0xe282b7 , None , u8"\xe2\x82\xb7" }, /* ₷ : up=₷ : lo=₷ */
  { 0xe282b8 , 0xe282b8 , 0xe282b8 , None , "KZT" }, /* ₸ : up=₸ : lo=₸ */
  { 0xe282b9 , 0xe282b9 , 0xe282b9 , None , "INR" }, /* ₹ : up=₹ : lo=₹ */
  { 0xe282ba , 0xe282ba , 0xe282ba , None , "TL" }, /* ₺ : up=₺ : lo=₺ */
  { 0xe282bb , 0xe282bb , 0xe282bb , None , u8"\xe2\x82\xbb" },
  { 0xe282bc , 0xe282bc , 0xe282bc , None , u8"\xe2\x82\xbc" },
  { 0xe282bd , 0xe282bd , 0xe282bd , None , "RUB" }, /* ₽ : up=₽ : lo=₽ */
  { 0xe282be , 0xe282be , 0xe282be , None , "GEL" },
  { 0xe282bf , 0xe282bf , 0xe282bf , None , u8"\xe2\x82\xbf" }, /* ₿ : up=₿ : lo=₿ */
};

const character charmap_e2_b4[64] = {
  { 0xe2b480 , 0xe182a0 , 0xe2b480 , None , u8"\xe2\xb4\x80" }, /* ⴀ : up=Ⴀ : lo=ⴀ */
  { 0xe2b481 , 0xe182a1 , 0xe2b481 , None , u8"\xe2\xb4\x81" }, /* ⴁ : up=Ⴁ : lo=ⴁ */
  { 0xe2b482 , 0xe182a2 , 0xe2b482 , None , u8"\xe2\xb4\x82" }, /* ⴂ : up=Ⴂ : lo=ⴂ */
  { 0xe2b483 , 0xe182a3 , 0xe2b483 , None , u8"\xe2\xb4\x83" }, /* ⴃ : up=Ⴃ : lo=ⴃ */
  { 0xe2b484 , 0xe182a4 , 0xe2b484 , None , u8"\xe2\xb4\x84" }, /* ⴄ : up=Ⴄ : lo=ⴄ */
  { 0xe2b485 , 0xe182a5 , 0xe2b485 , None , u8"\xe2\xb4\x85" }, /* ⴅ : up=Ⴅ : lo=ⴅ */
  { 0xe2b486 , 0xe182a6 , 0xe2b486 , None , u8"\xe2\xb4\x86" }, /* ⴆ : up=Ⴆ : lo=ⴆ */
  { 0xe2b487 , 0xe182a7 , 0xe2b487 , None , u8"\xe2\xb4\x87" }, /* ⴇ : up=Ⴇ : lo=ⴇ */
  { 0xe2b488 , 0xe182a8 , 0xe2b488 , None , u8"\xe2\xb4\x88" }, /* ⴈ : up=Ⴈ : lo=ⴈ */
  { 0xe2b489 , 0xe182a9 , 0xe2b489 , None , u8"\xe2\xb4\x89" }, /* ⴉ : up=Ⴉ : lo=ⴉ */
  { 0xe2b48a , 0xe182aa , 0xe2b48a , None , u8"\xe2\xb4\x8a" }, /* ⴊ : up=Ⴊ : lo=ⴊ */
  { 0xe2b48b , 0xe182ab , 0xe2b48b , None , u8"\xe2\xb4\x8b" }, /* ⴋ : up=Ⴋ : lo=ⴋ */
  { 0xe2b48c , 0xe182ac , 0xe2b48c , None , u8"\xe2\xb4\x8c" }, /* ⴌ : up=Ⴌ : lo=ⴌ */
  { 0xe2b48d , 0xe182ad , 0xe2b48d , None , u8"\xe2\xb4\x8d" }, /* ⴍ : up=Ⴍ : lo=ⴍ */
  { 0xe2b48e , 0xe182ae , 0xe2b48e , None , u8"\xe2\xb4\x8e" }, /* ⴎ : up=Ⴎ : lo=ⴎ */
  { 0xe2b48f , 0xe182af , 0xe2b48f , None , u8"\xe2\xb4\x8f" }, /* ⴏ : up=Ⴏ : lo=ⴏ */
  { 0xe2b490 , 0xe182b0 , 0xe2b490 , None , u8"\xe2\xb4\x90" }, /* ⴐ : up=Ⴐ : lo=ⴐ */
  { 0xe2b491 , 0xe182b1 , 0xe2b491 , None , u8"\xe2\xb4\x91" }, /* ⴑ : up=Ⴑ : lo=ⴑ */
  { 0xe2b492 , 0xe182b2 , 0xe2b492 , None , u8"\xe2\xb4\x92" }, /* ⴒ : up=Ⴒ : lo=ⴒ */
  { 0xe2b493 , 0xe182b3 , 0xe2b493 , None , u8"\xe2\xb4\x93" }, /* ⴓ : up=Ⴓ : lo=ⴓ */
  { 0xe2b494 , 0xe182b4 , 0xe2b494 , None , u8"\xe2\xb4\x94" }, /* ⴔ : up=Ⴔ : lo=ⴔ */
  { 0xe2b495 , 0xe182b5 , 0xe2b495 , None , u8"\xe2\xb4\x95" }, /* ⴕ : up=Ⴕ : lo=ⴕ */
  { 0xe2b496 , 0xe182b6 , 0xe2b496 , None , u8"\xe2\xb4\x96" }, /* ⴖ : up=Ⴖ : lo=ⴖ */
  { 0xe2b497 , 0xe182b7 , 0xe2b497 , None , u8"\xe2\xb4\x97" }, /* ⴗ : up=Ⴗ : lo=ⴗ */
  { 0xe2b498 , 0xe182b8 , 0xe2b498 , None , u8"\xe2\xb4\x98" }, /* ⴘ : up=Ⴘ : lo=ⴘ */
  { 0xe2b499 , 0xe182b9 , 0xe2b499 , None , u8"\xe2\xb4\x99" }, /* ⴙ : up=Ⴙ : lo=ⴙ */
  { 0xe2b49a , 0xe182ba , 0xe2b49a , None , u8"\xe2\xb4\x9a" }, /* ⴚ : up=Ⴚ : lo=ⴚ */
  { 0xe2b49b , 0xe182bb , 0xe2b49b , None , u8"\xe2\xb4\x9b" }, /* ⴛ : up=Ⴛ : lo=ⴛ */
  { 0xe2b49c , 0xe182bc , 0xe2b49c , None , u8"\xe2\xb4\x9c" }, /* ⴜ : up=Ⴜ : lo=ⴜ */
  { 0xe2b49d , 0xe182bd , 0xe2b49d , None , u8"\xe2\xb4\x9d" }, /* ⴝ : up=Ⴝ : lo=ⴝ */
  { 0xe2b49e , 0xe182be , 0xe2b49e , None , u8"\xe2\xb4\x9e" }, /* ⴞ : up=Ⴞ : lo=ⴞ */
  { 0xe2b49f , 0xe182bf , 0xe2b49f , None , u8"\xe2\xb4\x9f" }, /* ⴟ : up=Ⴟ : lo=ⴟ */
  { 0xe2b4a0 , 0xe18380 , 0xe2b4a0 , None , u8"\xe2\xb4\xa0" }, /* ⴠ : up=Ⴠ : lo=ⴠ */
  { 0xe2b4a1 , 0xe18381 , 0xe2b4a1 , None , u8"\xe2\xb4\xa1" }, /* ⴡ : up=Ⴡ : lo=ⴡ */
  { 0xe2b4a2 , 0xe18382 , 0xe2b4a2 , None , u8"\xe2\xb4\xa2" }, /* ⴢ : up=Ⴢ : lo=ⴢ */
  { 0xe2b4a3 , 0xe18383 , 0xe2b4a3 , None , u8"\xe2\xb4\xa3" }, /* ⴣ : up=Ⴣ : lo=ⴣ */
  { 0xe2b4a4 , 0xe18384 , 0xe2b4a4 , None , u8"\xe2\xb4\xa4" }, /* ⴤ : up=Ⴤ : lo=ⴤ */
  { 0xe2b4a5 , 0xe18385 , 0xe2b4a5 , None , u8"\xe2\xb4\xa5" }, /* ⴥ : up=Ⴥ : lo=ⴥ */
  { 0xe2b4a6 , 0xe2b4a6 , 0xe2b4a6 , None , u8"\xe2\xb4\xa6" }, /* ⴦ : up=⴦ : lo=⴦ */
  { 0xe2b4a7 , 0xe18387 , 0xe2b4a7 , None , u8"\xe2\xb4\xa7" }, /* ⴧ : up=Ⴧ : lo=ⴧ */
  { 0xe2b4a8 , 0xe2b4a8 , 0xe2b4a8 , None , u8"\xe2\xb4\xa8" }, /* ⴨ : up=⴨ : lo=⴨ */
  { 0xe2b4a9 , 0xe2b4a9 , 0xe2b4a9 , None , u8"\xe2\xb4\xa9" }, /* ⴩ : up=⴩ : lo=⴩ */
  { 0xe2b4aa , 0xe2b4aa , 0xe2b4aa , None , u8"\xe2\xb4\xaa" }, /* ⴪ : up=⴪ : lo=⴪ */
  { 0xe2b4ab , 0xe2b4ab , 0xe2b4ab , None , u8"\xe2\xb4\xab" }, /* ⴫ : up=⴫ : lo=⴫ */
  { 0xe2b4ac , 0xe2b4ac , 0xe2b4ac , None , u8"\xe2\xb4\xac" }, /* ⴬ : up=⴬ : lo=⴬ */
  { 0xe2b4ad , 0xe1838d , 0xe2b4ad , None , u8"\xe2\xb4\xad" }, /* ⴭ : up=Ⴭ : lo=ⴭ */
  { 0xe2b4ae , 0xe2b4ae , 0xe2b4ae , None , u8"\xe2\xb4\xae" }, /* ⴮ : up=⴮ : lo=⴮ */
  { 0xe2b4af , 0xe2b4af , 0xe2b4af , None , u8"\xe2\xb4\xaf" }, /* ⴯ : up=⴯ : lo=⴯ */
  { 0xe2b4b0 , 0xe2b4b0 , 0xe2b4b0 , None , u8"\xe2\xb4\xb0" }, /* ⴰ : up=ⴰ : lo=ⴰ */
  { 0xe2b4b1 , 0xe2b4b1 , 0xe2b4b1 , None , u8"\xe2\xb4\xb1" }, /* ⴱ : up=ⴱ : lo=ⴱ */
  { 0xe2b4b2 , 0xe2b4b2 , 0xe2b4b2 , None , u8"\xe2\xb4\xb2" }, /* ⴲ : up=ⴲ : lo=ⴲ */
  { 0xe2b4b3 , 0xe2b4b3 , 0xe2b4b3 , None , u8"\xe2\xb4\xb3" }, /* ⴳ : up=ⴳ : lo=ⴳ */
  { 0xe2b4b4 , 0xe2b4b4 , 0xe2b4b4 , None , u8"\xe2\xb4\xb4" }, /* ⴴ : up=ⴴ : lo=ⴴ */
  { 0xe2b4b5 , 0xe2b4b5 , 0xe2b4b5 , None , u8"\xe2\xb4\xb5" }, /* ⴵ : up=ⴵ : lo=ⴵ */
  { 0xe2b4b6 , 0xe2b4b6 , 0xe2b4b6 , None , u8"\xe2\xb4\xb6" }, /* ⴶ : up=ⴶ : lo=ⴶ */
  { 0xe2b4b7 , 0xe2b4b7 , 0xe2b4b7 , None , u8"\xe2\xb4\xb7" }, /* ⴷ : up=ⴷ : lo=ⴷ */
  { 0xe2b4b8 , 0xe2b4b8 , 0xe2b4b8 , None , u8"\xe2\xb4\xb8" }, /* ⴸ : up=ⴸ : lo=ⴸ */
  { 0xe2b4b9 , 0xe2b4b9 , 0xe2b4b9 , None , u8"\xe2\xb4\xb9" }, /* ⴹ : up=ⴹ : lo=ⴹ */
  { 0xe2b4ba , 0xe2b4ba , 0xe2b4ba , None , u8"\xe2\xb4\xba" }, /* ⴺ : up=ⴺ : lo=ⴺ */
  { 0xe2b4bb , 0xe2b4bb , 0xe2b4bb , None , u8"\xe2\xb4\xbb" }, /* ⴻ : up=ⴻ : lo=ⴻ */
  { 0xe2b4bc , 0xe2b4bc , 0xe2b4bc , None , u8"\xe2\xb4\xbc" }, /* ⴼ : up=ⴼ : lo=ⴼ */
  { 0xe2b4bd , 0xe2b4bd , 0xe2b4bd , None , u8"\xe2\xb4\xbd" }, /* ⴽ : up=ⴽ : lo=ⴽ */
  { 0xe2b4be , 0xe2b4be , 0xe2b4be , None , u8"\xe2\xb4\xbe" }, /* ⴾ : up=ⴾ : lo=ⴾ */
  { 0xe2b4bf , 0xe2b4bf , 0xe2b4bf , None , u8"\xe2\xb4\xbf" }, /* ⴿ : up=ⴿ : lo=ⴿ */
};

const character* pagemap_32_f0_90[64] = {
  /* 80 */
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, charmap_f0_90_92, charmap_f0_90_93, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

const character charmap_f0_90_92[64] = {
  { 0xf0909280 , 0xf0909280 , 0xf0909280 , None , u8"\xf0\x90\x92\x80" }, /* 𐒀 : up=𐒀 : lo=𐒀 */
  { 0xf0909281 , 0xf0909281 , 0xf0909281 , None , u8"\xf0\x90\x92\x81" }, /* 𐒁 : up=𐒁 : lo=𐒁 */
  { 0xf0909282 , 0xf0909282 , 0xf0909282 , None , u8"\xf0\x90\x92\x82" }, /* 𐒂 : up=𐒂 : lo=𐒂 */
  { 0xf0909283 , 0xf0909283 , 0xf0909283 , None , u8"\xf0\x90\x92\x83" }, /* 𐒃 : up=𐒃 : lo=𐒃 */
  { 0xf0909284 , 0xf0909284 , 0xf0909284 , None , u8"\xf0\x90\x92\x84" }, /* 𐒄 : up=𐒄 : lo=𐒄 */
  { 0xf0909285 , 0xf0909285 , 0xf0909285 , None , u8"\xf0\x90\x92\x85" }, /* 𐒅 : up=𐒅 : lo=𐒅 */
  { 0xf0909286 , 0xf0909286 , 0xf0909286 , None , u8"\xf0\x90\x92\x86" }, /* 𐒆 : up=𐒆 : lo=𐒆 */
  { 0xf0909287 , 0xf0909287 , 0xf0909287 , None , u8"\xf0\x90\x92\x87" }, /* 𐒇 : up=𐒇 : lo=𐒇 */
  { 0xf0909288 , 0xf0909288 , 0xf0909288 , None , u8"\xf0\x90\x92\x88" }, /* 𐒈 : up=𐒈 : lo=𐒈 */
  { 0xf0909289 , 0xf0909289 , 0xf0909289 , None , u8"\xf0\x90\x92\x89" }, /* 𐒉 : up=𐒉 : lo=𐒉 */
  { 0xf090928a , 0xf090928a , 0xf090928a , None , u8"\xf0\x90\x92\x8a" }, /* 𐒊 : up=𐒊 : lo=𐒊 */
  { 0xf090928b , 0xf090928b , 0xf090928b , None , u8"\xf0\x90\x92\x8b" }, /* 𐒋 : up=𐒋 : lo=𐒋 */
  { 0xf090928c , 0xf090928c , 0xf090928c , None , u8"\xf0\x90\x92\x8c" }, /* 𐒌 : up=𐒌 : lo=𐒌 */
  { 0xf090928d , 0xf090928d , 0xf090928d , None , u8"\xf0\x90\x92\x8d" }, /* 𐒍 : up=𐒍 : lo=𐒍 */
  { 0xf090928e , 0xf090928e , 0xf090928e , None , u8"\xf0\x90\x92\x8e" }, /* 𐒎 : up=𐒎 : lo=𐒎 */
  { 0xf090928f , 0xf090928f , 0xf090928f , None , u8"\xf0\x90\x92\x8f" }, /* 𐒏 : up=𐒏 : lo=𐒏 */
  { 0xf0909290 , 0xf0909290 , 0xf0909290 , None , u8"\xf0\x90\x92\x90" }, /* 𐒐 : up=𐒐 : lo=𐒐 */
  { 0xf0909291 , 0xf0909291 , 0xf0909291 , None , u8"\xf0\x90\x92\x91" }, /* 𐒑 : up=𐒑 : lo=𐒑 */
  { 0xf0909292 , 0xf0909292 , 0xf0909292 , None , u8"\xf0\x90\x92\x92" }, /* 𐒒 : up=𐒒 : lo=𐒒 */
  { 0xf0909293 , 0xf0909293 , 0xf0909293 , None , u8"\xf0\x90\x92\x93" }, /* 𐒓 : up=𐒓 : lo=𐒓 */
  { 0xf0909294 , 0xf0909294 , 0xf0909294 , None , u8"\xf0\x90\x92\x94" }, /* 𐒔 : up=𐒔 : lo=𐒔 */
  { 0xf0909295 , 0xf0909295 , 0xf0909295 , None , u8"\xf0\x90\x92\x95" }, /* 𐒕 : up=𐒕 : lo=𐒕 */
  { 0xf0909296 , 0xf0909296 , 0xf0909296 , None , u8"\xf0\x90\x92\x96" }, /* 𐒖 : up=𐒖 : lo=𐒖 */
  { 0xf0909297 , 0xf0909297 , 0xf0909297 , None , u8"\xf0\x90\x92\x97" }, /* 𐒗 : up=𐒗 : lo=𐒗 */
  { 0xf0909298 , 0xf0909298 , 0xf0909298 , None , u8"\xf0\x90\x92\x98" }, /* 𐒘 : up=𐒘 : lo=𐒘 */
  { 0xf0909299 , 0xf0909299 , 0xf0909299 , None , u8"\xf0\x90\x92\x99" }, /* 𐒙 : up=𐒙 : lo=𐒙 */
  { 0xf090929a , 0xf090929a , 0xf090929a , None , u8"\xf0\x90\x92\x9a" }, /* 𐒚 : up=𐒚 : lo=𐒚 */
  { 0xf090929b , 0xf090929b , 0xf090929b , None , u8"\xf0\x90\x92\x9b" }, /* 𐒛 : up=𐒛 : lo=𐒛 */
  { 0xf090929c , 0xf090929c , 0xf090929c , None , u8"\xf0\x90\x92\x9c" }, /* 𐒜 : up=𐒜 : lo=𐒜 */
  { 0xf090929d , 0xf090929d , 0xf090929d , None , u8"\xf0\x90\x92\x9d" }, /* 𐒝 : up=𐒝 : lo=𐒝 */
  { 0xf090929e , 0xf090929e , 0xf090929e , None , u8"\xf0\x90\x92\x9e" }, /* 𐒞 : up=𐒞 : lo=𐒞 */
  { 0xf090929f , 0xf090929f , 0xf090929f , None , u8"\xf0\x90\x92\x9f" }, /* 𐒟 : up=𐒟 : lo=𐒟 */
  { 0xf09092a0 , 0xf09092a0 , 0xf09092a0 , None , u8"\xf0\x90\x92\xa0" }, /* 𐒠 : up=𐒠 : lo=𐒠 */
  { 0xf09092a1 , 0xf09092a1 , 0xf09092a1 , None , u8"\xf0\x90\x92\xa1" }, /* 𐒡 : up=𐒡 : lo=𐒡 */
  { 0xf09092a2 , 0xf09092a2 , 0xf09092a2 , None , u8"\xf0\x90\x92\xa2" }, /* 𐒢 : up=𐒢 : lo=𐒢 */
  { 0xf09092a3 , 0xf09092a3 , 0xf09092a3 , None , u8"\xf0\x90\x92\xa3" }, /* 𐒣 : up=𐒣 : lo=𐒣 */
  { 0xf09092a4 , 0xf09092a4 , 0xf09092a4 , None , u8"\xf0\x90\x92\xa4" }, /* 𐒤 : up=𐒤 : lo=𐒤 */
  { 0xf09092a5 , 0xf09092a5 , 0xf09092a5 , None , u8"\xf0\x90\x92\xa5" }, /* 𐒥 : up=𐒥 : lo=𐒥 */
  { 0xf09092a6 , 0xf09092a6 , 0xf09092a6 , None , u8"\xf0\x90\x92\xa6" }, /* 𐒦 : up=𐒦 : lo=𐒦 */
  { 0xf09092a7 , 0xf09092a7 , 0xf09092a7 , None , u8"\xf0\x90\x92\xa7" }, /* 𐒧 : up=𐒧 : lo=𐒧 */
  { 0xf09092a8 , 0xf09092a8 , 0xf09092a8 , None , u8"\xf0\x90\x92\xa8" }, /* 𐒨 : up=𐒨 : lo=𐒨 */
  { 0xf09092a9 , 0xf09092a9 , 0xf09092a9 , None , u8"\xf0\x90\x92\xa9" }, /* 𐒩 : up=𐒩 : lo=𐒩 */
  { 0xf09092aa , 0xf09092aa , 0xf09092aa , None , u8"\xf0\x90\x92\xaa" }, /* 𐒪 : up=𐒪 : lo=𐒪 */
  { 0xf09092ab , 0xf09092ab , 0xf09092ab , None , u8"\xf0\x90\x92\xab" }, /* 𐒫 : up=𐒫 : lo=𐒫 */
  { 0xf09092ac , 0xf09092ac , 0xf09092ac , None , u8"\xf0\x90\x92\xac" }, /* 𐒬 : up=𐒬 : lo=𐒬 */
  { 0xf09092ad , 0xf09092ad , 0xf09092ad , None , u8"\xf0\x90\x92\xad" }, /* 𐒭 : up=𐒭 : lo=𐒭 */
  { 0xf09092ae , 0xf09092ae , 0xf09092ae , None , u8"\xf0\x90\x92\xae" }, /* 𐒮 : up=𐒮 : lo=𐒮 */
  { 0xf09092af , 0xf09092af , 0xf09092af , None , u8"\xf0\x90\x92\xaf" }, /* 𐒯 : up=𐒯 : lo=𐒯 */
  { 0xf09092b0 , 0xf09092b0 , 0xf0909398 , None , u8"\xf0\x90\x92\xb0" }, /* 𐒰 : up=𐒰 : lo=𐓘 */
  { 0xf09092b1 , 0xf09092b1 , 0xf0909399 , None , u8"\xf0\x90\x92\xb1" }, /* 𐒱 : up=𐒱 : lo=𐓙 */
  { 0xf09092b2 , 0xf09092b2 , 0xf090939a , None , u8"\xf0\x90\x92\xb2" }, /* 𐒲 : up=𐒲 : lo=𐓚 */
  { 0xf09092b3 , 0xf09092b3 , 0xf090939b , None , u8"\xf0\x90\x92\xb3" }, /* 𐒳 : up=𐒳 : lo=𐓛 */
  { 0xf09092b4 , 0xf09092b4 , 0xf090939c , None , u8"\xf0\x90\x92\xb4" }, /* 𐒴 : up=𐒴 : lo=𐓜 */
  { 0xf09092b5 , 0xf09092b5 , 0xf090939d , None , u8"\xf0\x90\x92\xb5" }, /* 𐒵 : up=𐒵 : lo=𐓝 */
  { 0xf09092b6 , 0xf09092b6 , 0xf090939e , None , u8"\xf0\x90\x92\xb6" }, /* 𐒶 : up=𐒶 : lo=𐓞 */
  { 0xf09092b7 , 0xf09092b7 , 0xf090939f , None , u8"\xf0\x90\x92\xb7" }, /* 𐒷 : up=𐒷 : lo=𐓟 */
  { 0xf09092b8 , 0xf09092b8 , 0xf09093a0 , None , u8"\xf0\x90\x92\xb8" }, /* 𐒸 : up=𐒸 : lo=𐓠 */
  { 0xf09092b9 , 0xf09092b9 , 0xf09093a1 , None , u8"\xf0\x90\x92\xb9" }, /* 𐒹 : up=𐒹 : lo=𐓡 */
  { 0xf09092ba , 0xf09092ba , 0xf09093a2 , None , u8"\xf0\x90\x92\xba" }, /* 𐒺 : up=𐒺 : lo=𐓢 */
  { 0xf09092bb , 0xf09092bb , 0xf09093a3 , None , u8"\xf0\x90\x92\xbb" }, /* 𐒻 : up=𐒻 : lo=𐓣 */
  { 0xf09092bc , 0xf09092bc , 0xf09093a4 , None , u8"\xf0\x90\x92\xbc" }, /* 𐒼 : up=𐒼 : lo=𐓤 */
  { 0xf09092bd , 0xf09092bd , 0xf09093a5 , None , u8"\xf0\x90\x92\xbd" }, /* 𐒽 : up=𐒽 : lo=𐓥 */
  { 0xf09092be , 0xf09092be , 0xf09093a6 , None , u8"\xf0\x90\x92\xbe" }, /* 𐒾 : up=𐒾 : lo=𐓦 */
  { 0xf09092bf , 0xf09092bf , 0xf09093a7 , None , u8"\xf0\x90\x92\xbf" }, /* 𐒿 : up=𐒿 : lo=𐓧 */
};

const character charmap_f0_90_93[64] = {
  { 0xf0909380 , 0xf0909380 , 0xf09093a8 , None , u8"\xf0\x90\x93\x80" }, /* 𐓀 : up=𐓀 : lo=𐓨 */
  { 0xf0909381 , 0xf0909381 , 0xf09093a9 , None , u8"\xf0\x90\x93\x81" }, /* 𐓁 : up=𐓁 : lo=𐓩 */
  { 0xf0909382 , 0xf0909382 , 0xf09093aa , None , u8"\xf0\x90\x93\x82" }, /* 𐓂 : up=𐓂 : lo=𐓪 */
  { 0xf0909383 , 0xf0909383 , 0xf09093ab , None , u8"\xf0\x90\x93\x83" }, /* 𐓃 : up=𐓃 : lo=𐓫 */
  { 0xf0909384 , 0xf0909384 , 0xf09093ac , None , u8"\xf0\x90\x93\x84" }, /* 𐓄 : up=𐓄 : lo=𐓬 */
  { 0xf0909385 , 0xf0909385 , 0xf09093ad , None , u8"\xf0\x90\x93\x85" }, /* 𐓅 : up=𐓅 : lo=𐓭 */
  { 0xf0909386 , 0xf0909386 , 0xf09093ae , None , u8"\xf0\x90\x93\x86" }, /* 𐓆 : up=𐓆 : lo=𐓮 */
  { 0xf0909387 , 0xf0909387 , 0xf09093af , None , u8"\xf0\x90\x93\x87" }, /* 𐓇 : up=𐓇 : lo=𐓯 */
  { 0xf0909388 , 0xf0909388 , 0xf09093b0 , None , u8"\xf0\x90\x93\x88" }, /* 𐓈 : up=𐓈 : lo=𐓰 */
  { 0xf0909389 , 0xf0909389 , 0xf09093b1 , None , u8"\xf0\x90\x93\x89" }, /* 𐓉 : up=𐓉 : lo=𐓱 */
  { 0xf090938a , 0xf090938a , 0xf09093b2 , None , u8"\xf0\x90\x93\x8a" }, /* 𐓊 : up=𐓊 : lo=𐓲 */
  { 0xf090938b , 0xf090938b , 0xf09093b3 , None , u8"\xf0\x90\x93\x8b" }, /* 𐓋 : up=𐓋 : lo=𐓳 */
  { 0xf090938c , 0xf090938c , 0xf09093b4 , None , u8"\xf0\x90\x93\x8c" }, /* 𐓌 : up=𐓌 : lo=𐓴 */
  { 0xf090938d , 0xf090938d , 0xf09093b5 , None , u8"\xf0\x90\x93\x8d" }, /* 𐓍 : up=𐓍 : lo=𐓵 */
  { 0xf090938e , 0xf090938e , 0xf09093b6 , None , u8"\xf0\x90\x93\x8e" }, /* 𐓎 : up=𐓎 : lo=𐓶 */
  { 0xf090938f , 0xf090938f , 0xf09093b7 , None , u8"\xf0\x90\x93\x8f" }, /* 𐓏 : up=𐓏 : lo=𐓷 */
  { 0xf0909390 , 0xf0909390 , 0xf09093b8 , None , u8"\xf0\x90\x93\x90" }, /* 𐓐 : up=𐓐 : lo=𐓸 */
  { 0xf0909391 , 0xf0909391 , 0xf09093b9 , None , u8"\xf0\x90\x93\x91" }, /* 𐓑 : up=𐓑 : lo=𐓹 */
  { 0xf0909392 , 0xf0909392 , 0xf09093ba , None , u8"\xf0\x90\x93\x92" }, /* 𐓒 : up=𐓒 : lo=𐓺 */
  { 0xf0909393 , 0xf0909393 , 0xf09093bb , None , u8"\xf0\x90\x93\x93" }, /* 𐓓 : up=𐓓 : lo=𐓻 */
  { 0xf0909394 , 0xf0909394 , 0xf0909394 , None , u8"\xf0\x90\x93\x94" }, /* 𐓔 : up=𐓔 : lo=𐓔 */
  { 0xf0909395 , 0xf0909395 , 0xf0909395 , None , u8"\xf0\x90\x93\x95" }, /* 𐓕 : up=𐓕 : lo=𐓕 */
  { 0xf0909396 , 0xf0909396 , 0xf0909396 , None , u8"\xf0\x90\x93\x96" }, /* 𐓖 : up=𐓖 : lo=𐓖 */
  { 0xf0909397 , 0xf0909397 , 0xf0909397 , None , u8"\xf0\x90\x93\x97" }, /* 𐓗 : up=𐓗 : lo=𐓗 */
  { 0xf0909398 , 0xf09092b0 , 0xf0909398 , None , u8"\xf0\x90\x93\x98" }, /* 𐓘 : up=𐒰 : lo=𐓘 */
  { 0xf0909399 , 0xf09092b1 , 0xf0909399 , None , u8"\xf0\x90\x93\x99" }, /* 𐓙 : up=𐒱 : lo=𐓙 */
  { 0xf090939a , 0xf09092b2 , 0xf090939a , None , u8"\xf0\x90\x93\x9a" }, /* 𐓚 : up=𐒲 : lo=𐓚 */
  { 0xf090939b , 0xf09092b3 , 0xf090939b , None , u8"\xf0\x90\x93\x9b" }, /* 𐓛 : up=𐒳 : lo=𐓛 */
  { 0xf090939c , 0xf09092b4 , 0xf090939c , None , u8"\xf0\x90\x93\x9c" }, /* 𐓜 : up=𐒴 : lo=𐓜 */
  { 0xf090939d , 0xf09092b5 , 0xf090939d , None , u8"\xf0\x90\x93\x9d" }, /* 𐓝 : up=𐒵 : lo=𐓝 */
  { 0xf090939e , 0xf09092b6 , 0xf090939e , None , u8"\xf0\x90\x93\x9e" }, /* 𐓞 : up=𐒶 : lo=𐓞 */
  { 0xf090939f , 0xf09092b7 , 0xf090939f , None , u8"\xf0\x90\x93\x9f" }, /* 𐓟 : up=𐒷 : lo=𐓟 */
  { 0xf09093a0 , 0xf09092b8 , 0xf09093a0 , None , u8"\xf0\x90\x93\xa0" }, /* 𐓠 : up=𐒸 : lo=𐓠 */
  { 0xf09093a1 , 0xf09092b9 , 0xf09093a1 , None , u8"\xf0\x90\x93\xa1" }, /* 𐓡 : up=𐒹 : lo=𐓡 */
  { 0xf09093a2 , 0xf09092ba , 0xf09093a2 , None , u8"\xf0\x90\x93\xa2" }, /* 𐓢 : up=𐒺 : lo=𐓢 */
  { 0xf09093a3 , 0xf09092bb , 0xf09093a3 , None , u8"\xf0\x90\x93\xa3" }, /* 𐓣 : up=𐒻 : lo=𐓣 */
  { 0xf09093a4 , 0xf09092bc , 0xf09093a4 , None , u8"\xf0\x90\x93\xa4" }, /* 𐓤 : up=𐒼 : lo=𐓤 */
  { 0xf09093a5 , 0xf09092bd , 0xf09093a5 , None , u8"\xf0\x90\x93\xa5" }, /* 𐓥 : up=𐒽 : lo=𐓥 */
  { 0xf09093a6 , 0xf09092be , 0xf09093a6 , None , u8"\xf0\x90\x93\xa6" }, /* 𐓦 : up=𐒾 : lo=𐓦 */
  { 0xf09093a7 , 0xf09092bf , 0xf09093a7 , None , u8"\xf0\x90\x93\xa7" }, /* 𐓧 : up=𐒿 : lo=𐓧 */
  { 0xf09093a8 , 0xf0909380 , 0xf09093a8 , None , u8"\xf0\x90\x93\xa8" }, /* 𐓨 : up=𐓀 : lo=𐓨 */
  { 0xf09093a9 , 0xf0909381 , 0xf09093a9 , None , u8"\xf0\x90\x93\xa9" }, /* 𐓩 : up=𐓁 : lo=𐓩 */
  { 0xf09093aa , 0xf0909382 , 0xf09093aa , None , u8"\xf0\x90\x93\xaa" }, /* 𐓪 : up=𐓂 : lo=𐓪 */
  { 0xf09093ab , 0xf0909383 , 0xf09093ab , None , u8"\xf0\x90\x93\xab" }, /* 𐓫 : up=𐓃 : lo=𐓫 */
  { 0xf09093ac , 0xf0909384 , 0xf09093ac , None , u8"\xf0\x90\x93\xac" }, /* 𐓬 : up=𐓄 : lo=𐓬 */
  { 0xf09093ad , 0xf0909385 , 0xf09093ad , None , u8"\xf0\x90\x93\xad" }, /* 𐓭 : up=𐓅 : lo=𐓭 */
  { 0xf09093ae , 0xf0909386 , 0xf09093ae , None , u8"\xf0\x90\x93\xae" }, /* 𐓮 : up=𐓆 : lo=𐓮 */
  { 0xf09093af , 0xf0909387 , 0xf09093af , None , u8"\xf0\x90\x93\xaf" }, /* 𐓯 : up=𐓇 : lo=𐓯 */
  { 0xf09093b0 , 0xf0909388 , 0xf09093b0 , None , u8"\xf0\x90\x93\xb0" }, /* 𐓰 : up=𐓈 : lo=𐓰 */
  { 0xf09093b1 , 0xf0909389 , 0xf09093b1 , None , u8"\xf0\x90\x93\xb1" }, /* 𐓱 : up=𐓉 : lo=𐓱 */
  { 0xf09093b2 , 0xf090938a , 0xf09093b2 , None , u8"\xf0\x90\x93\xb2" }, /* 𐓲 : up=𐓊 : lo=𐓲 */
  { 0xf09093b3 , 0xf090938b , 0xf09093b3 , None , u8"\xf0\x90\x93\xb3" }, /* 𐓳 : up=𐓋 : lo=𐓳 */
  { 0xf09093b4 , 0xf090938c , 0xf09093b4 , None , u8"\xf0\x90\x93\xb4" }, /* 𐓴 : up=𐓌 : lo=𐓴 */
  { 0xf09093b5 , 0xf090938d , 0xf09093b5 , None , u8"\xf0\x90\x93\xb5" }, /* 𐓵 : up=𐓍 : lo=𐓵 */
  { 0xf09093b6 , 0xf090938e , 0xf09093b6 , None , u8"\xf0\x90\x93\xb6" }, /* 𐓶 : up=𐓎 : lo=𐓶 */
  { 0xf09093b7 , 0xf090938f , 0xf09093b7 , None , u8"\xf0\x90\x93\xb7" }, /* 𐓷 : up=𐓏 : lo=𐓷 */
  { 0xf09093b8 , 0xf0909390 , 0xf09093b8 , None , u8"\xf0\x90\x93\xb8" }, /* 𐓸 : up=𐓐 : lo=𐓸 */
  { 0xf09093b9 , 0xf0909391 , 0xf09093b9 , None , u8"\xf0\x90\x93\xb9" }, /* 𐓹 : up=𐓑 : lo=𐓹 */
  { 0xf09093ba , 0xf0909392 , 0xf09093ba , None , u8"\xf0\x90\x93\xba" }, /* 𐓺 : up=𐓒 : lo=𐓺 */
  { 0xf09093bb , 0xf0909393 , 0xf09093bb , None , u8"\xf0\x90\x93\xbb" }, /* 𐓻 : up=𐓓 : lo=𐓻 */
  { 0xf09093bc , 0xf09093bc , 0xf09093bc , None , u8"\xf0\x90\x93\xbc" }, /* 𐓼 : up=𐓼 : lo=𐓼 */
  { 0xf09093bd , 0xf09093bd , 0xf09093bd , None , u8"\xf0\x90\x93\xbd" }, /* 𐓽 : up=𐓽 : lo=𐓽 */
  { 0xf09093be , 0xf09093be , 0xf09093be , None , u8"\xf0\x90\x93\xbe" }, /* 𐓾 : up=𐓾 : lo=𐓾 */
  { 0xf09093bf , 0xf09093bf , 0xf09093bf , None , u8"\xf0\x90\x93\xbf" }, /* 𐓿 : up=𐓿 : lo=𐓿 */
};

const character* pagemap_32_f0_9e[64] = {
  /* 80 */
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, charmap_f0_9e_a4, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

const character charmap_f0_9e_a4[64] = {
  { 0xf09ea480 , 0xf09ea480 , 0xf09ea4a2 , None , u8"\xf0\x9e\xa4\x80" }, /* 𞤀 : up=𞤀 : lo=𞤢 */
  { 0xf09ea481 , 0xf09ea481 , 0xf09ea4a3 , None , u8"\xf0\x9e\xa4\x81" }, /* 𞤁 : up=𞤁 : lo=𞤣 */
  { 0xf09ea482 , 0xf09ea482 , 0xf09ea4a4 , None , u8"\xf0\x9e\xa4\x82" }, /* 𞤂 : up=𞤂 : lo=𞤤 */
  { 0xf09ea483 , 0xf09ea483 , 0xf09ea4a5 , None , u8"\xf0\x9e\xa4\x83" }, /* 𞤃 : up=𞤃 : lo=𞤥 */
  { 0xf09ea484 , 0xf09ea484 , 0xf09ea4a6 , None , u8"\xf0\x9e\xa4\x84" }, /* 𞤄 : up=𞤄 : lo=𞤦 */
  { 0xf09ea485 , 0xf09ea485 , 0xf09ea4a7 , None , u8"\xf0\x9e\xa4\x85" }, /* 𞤅 : up=𞤅 : lo=𞤧 */
  { 0xf09ea486 , 0xf09ea486 , 0xf09ea4a8 , None , u8"\xf0\x9e\xa4\x86" }, /* 𞤆 : up=𞤆 : lo=𞤨 */
  { 0xf09ea487 , 0xf09ea487 , 0xf09ea4a9 , None , u8"\xf0\x9e\xa4\x87" }, /* 𞤇 : up=𞤇 : lo=𞤩 */
  { 0xf09ea488 , 0xf09ea488 , 0xf09ea4aa , None , u8"\xf0\x9e\xa4\x88" }, /* 𞤈 : up=𞤈 : lo=𞤪 */
  { 0xf09ea489 , 0xf09ea489 , 0xf09ea4ab , None , u8"\xf0\x9e\xa4\x89" }, /* 𞤉 : up=𞤉 : lo=𞤫 */
  { 0xf09ea48a , 0xf09ea48a , 0xf09ea4ac , None , u8"\xf0\x9e\xa4\x8a" }, /* 𞤊 : up=𞤊 : lo=𞤬 */
  { 0xf09ea48b , 0xf09ea48b , 0xf09ea4ad , None , u8"\xf0\x9e\xa4\x8b" }, /* 𞤋 : up=𞤋 : lo=𞤭 */
  { 0xf09ea48c , 0xf09ea48c , 0xf09ea4ae , None , u8"\xf0\x9e\xa4\x8c" }, /* 𞤌 : up=𞤌 : lo=𞤮 */
  { 0xf09ea48d , 0xf09ea48d , 0xf09ea4af , None , u8"\xf0\x9e\xa4\x8d" }, /* 𞤍 : up=𞤍 : lo=𞤯 */
  { 0xf09ea48e , 0xf09ea48e , 0xf09ea4b0 , None , u8"\xf0\x9e\xa4\x8e" }, /* 𞤎 : up=𞤎 : lo=𞤰 */
  { 0xf09ea48f , 0xf09ea48f , 0xf09ea4b1 , None , u8"\xf0\x9e\xa4\x8f" }, /* 𞤏 : up=𞤏 : lo=𞤱 */
  { 0xf09ea490 , 0xf09ea490 , 0xf09ea4b2 , None , u8"\xf0\x9e\xa4\x90" }, /* 𞤐 : up=𞤐 : lo=𞤲 */
  { 0xf09ea491 , 0xf09ea491 , 0xf09ea4b3 , None , u8"\xf0\x9e\xa4\x91" }, /* 𞤑 : up=𞤑 : lo=𞤳 */
  { 0xf09ea492 , 0xf09ea492 , 0xf09ea4b4 , None , u8"\xf0\x9e\xa4\x92" }, /* 𞤒 : up=𞤒 : lo=𞤴 */
  { 0xf09ea493 , 0xf09ea493 , 0xf09ea4b5 , None , u8"\xf0\x9e\xa4\x93" }, /* 𞤓 : up=𞤓 : lo=𞤵 */
  { 0xf09ea494 , 0xf09ea494 , 0xf09ea4b6 , None , u8"\xf0\x9e\xa4\x94" }, /* 𞤔 : up=𞤔 : lo=𞤶 */
  { 0xf09ea495 , 0xf09ea495 , 0xf09ea4b7 , None , u8"\xf0\x9e\xa4\x95" }, /* 𞤕 : up=𞤕 : lo=𞤷 */
  { 0xf09ea496 , 0xf09ea496 , 0xf09ea4b8 , None , u8"\xf0\x9e\xa4\x96" }, /* 𞤖 : up=𞤖 : lo=𞤸 */
  { 0xf09ea497 , 0xf09ea497 , 0xf09ea4b9 , None , u8"\xf0\x9e\xa4\x97" }, /* 𞤗 : up=𞤗 : lo=𞤹 */
  { 0xf09ea498 , 0xf09ea498 , 0xf09ea4ba , None , u8"\xf0\x9e\xa4\x98" }, /* 𞤘 : up=𞤘 : lo=𞤺 */
  { 0xf09ea499 , 0xf09ea499 , 0xf09ea4bb , None , u8"\xf0\x9e\xa4\x99" }, /* 𞤙 : up=𞤙 : lo=𞤻 */
  { 0xf09ea49a , 0xf09ea49a , 0xf09ea4bc , None , u8"\xf0\x9e\xa4\x9a" }, /* 𞤚 : up=𞤚 : lo=𞤼 */
  { 0xf09ea49b , 0xf09ea49b , 0xf09ea4bd , None , u8"\xf0\x9e\xa4\x9b" }, /* 𞤛 : up=𞤛 : lo=𞤽 */
  { 0xf09ea49c , 0xf09ea49c , 0xf09ea4be , None , u8"\xf0\x9e\xa4\x9c" }, /* 𞤜 : up=𞤜 : lo=𞤾 */
  { 0xf09ea49d , 0xf09ea49d , 0xf09ea4bf , None , u8"\xf0\x9e\xa4\x9d" }, /* 𞤝 : up=𞤝 : lo=𞤿 */
  { 0xf09ea49e , 0xf09ea49e , 0xf09ea580 , None , u8"\xf0\x9e\xa4\x9e" }, /* 𞤞 : up=𞤞 : lo=𞥀 */
  { 0xf09ea49f , 0xf09ea49f , 0xf09ea581 , None , u8"\xf0\x9e\xa4\x9f" }, /* 𞤟 : up=𞤟 : lo=𞥁 */
  { 0xf09ea4a0 , 0xf09ea4a0 , 0xf09ea582 , None , u8"\xf0\x9e\xa4\xa0" }, /* 𞤠 : up=𞤠 : lo=𞥂 */
  { 0xf09ea4a1 , 0xf09ea4a1 , 0xf09ea583 , None , u8"\xf0\x9e\xa4\xa1" }, /* 𞤡 : up=𞤡 : lo=𞥃 */
  { 0xf09ea4a2 , 0xf09ea480 , 0xf09ea4a2 , None , u8"\xf0\x9e\xa4\xa2" }, /* 𞤢 : up=𞤀 : lo=𞤢 */
  { 0xf09ea4a3 , 0xf09ea481 , 0xf09ea4a3 , None , u8"\xf0\x9e\xa4\xa3" }, /* 𞤣 : up=𞤁 : lo=𞤣 */
  { 0xf09ea4a4 , 0xf09ea482 , 0xf09ea4a4 , None , u8"\xf0\x9e\xa4\xa4" }, /* 𞤤 : up=𞤂 : lo=𞤤 */
  { 0xf09ea4a5 , 0xf09ea483 , 0xf09ea4a5 , None , u8"\xf0\x9e\xa4\xa5" }, /* 𞤥 : up=𞤃 : lo=𞤥 */
  { 0xf09ea4a6 , 0xf09ea484 , 0xf09ea4a6 , None , u8"\xf0\x9e\xa4\xa6" }, /* 𞤦 : up=𞤄 : lo=𞤦 */
  { 0xf09ea4a7 , 0xf09ea485 , 0xf09ea4a7 , None , u8"\xf0\x9e\xa4\xa7" }, /* 𞤧 : up=𞤅 : lo=𞤧 */
  { 0xf09ea4a8 , 0xf09ea486 , 0xf09ea4a8 , None , u8"\xf0\x9e\xa4\xa8" }, /* 𞤨 : up=𞤆 : lo=𞤨 */
  { 0xf09ea4a9 , 0xf09ea487 , 0xf09ea4a9 , None , u8"\xf0\x9e\xa4\xa9" }, /* 𞤩 : up=𞤇 : lo=𞤩 */
  { 0xf09ea4aa , 0xf09ea488 , 0xf09ea4aa , None , u8"\xf0\x9e\xa4\xaa" }, /* 𞤪 : up=𞤈 : lo=𞤪 */
  { 0xf09ea4ab , 0xf09ea489 , 0xf09ea4ab , None , u8"\xf0\x9e\xa4\xab" }, /* 𞤫 : up=𞤉 : lo=𞤫 */
  { 0xf09ea4ac , 0xf09ea48a , 0xf09ea4ac , None , u8"\xf0\x9e\xa4\xac" }, /* 𞤬 : up=𞤊 : lo=𞤬 */
  { 0xf09ea4ad , 0xf09ea48b , 0xf09ea4ad , None , u8"\xf0\x9e\xa4\xad" }, /* 𞤭 : up=𞤋 : lo=𞤭 */
  { 0xf09ea4ae , 0xf09ea48c , 0xf09ea4ae , None , u8"\xf0\x9e\xa4\xae" }, /* 𞤮 : up=𞤌 : lo=𞤮 */
  { 0xf09ea4af , 0xf09ea48d , 0xf09ea4af , None , u8"\xf0\x9e\xa4\xaf" }, /* 𞤯 : up=𞤍 : lo=𞤯 */
  { 0xf09ea4b0 , 0xf09ea48e , 0xf09ea4b0 , None , u8"\xf0\x9e\xa4\xb0" }, /* 𞤰 : up=𞤎 : lo=𞤰 */
  { 0xf09ea4b1 , 0xf09ea48f , 0xf09ea4b1 , None , u8"\xf0\x9e\xa4\xb1" }, /* 𞤱 : up=𞤏 : lo=𞤱 */
  { 0xf09ea4b2 , 0xf09ea490 , 0xf09ea4b2 , None , u8"\xf0\x9e\xa4\xb2" }, /* 𞤲 : up=𞤐 : lo=𞤲 */
  { 0xf09ea4b3 , 0xf09ea491 , 0xf09ea4b3 , None , u8"\xf0\x9e\xa4\xb3" }, /* 𞤳 : up=𞤑 : lo=𞤳 */
  { 0xf09ea4b4 , 0xf09ea492 , 0xf09ea4b4 , None , u8"\xf0\x9e\xa4\xb4" }, /* 𞤴 : up=𞤒 : lo=𞤴 */
  { 0xf09ea4b5 , 0xf09ea493 , 0xf09ea4b5 , None , u8"\xf0\x9e\xa4\xb5" }, /* 𞤵 : up=𞤓 : lo=𞤵 */
  { 0xf09ea4b6 , 0xf09ea494 , 0xf09ea4b6 , None , u8"\xf0\x9e\xa4\xb6" }, /* 𞤶 : up=𞤔 : lo=𞤶 */
  { 0xf09ea4b7 , 0xf09ea495 , 0xf09ea4b7 , None , u8"\xf0\x9e\xa4\xb7" }, /* 𞤷 : up=𞤕 : lo=𞤷 */
  { 0xf09ea4b8 , 0xf09ea496 , 0xf09ea4b8 , None , u8"\xf0\x9e\xa4\xb8" }, /* 𞤸 : up=𞤖 : lo=𞤸 */
  { 0xf09ea4b9 , 0xf09ea497 , 0xf09ea4b9 , None , u8"\xf0\x9e\xa4\xb9" }, /* 𞤹 : up=𞤗 : lo=𞤹 */
  { 0xf09ea4ba , 0xf09ea498 , 0xf09ea4ba , None , u8"\xf0\x9e\xa4\xba" }, /* 𞤺 : up=𞤘 : lo=𞤺 */
  { 0xf09ea4bb , 0xf09ea499 , 0xf09ea4bb , None , u8"\xf0\x9e\xa4\xbb" }, /* 𞤻 : up=𞤙 : lo=𞤻 */
  { 0xf09ea4bc , 0xf09ea49a , 0xf09ea4bc , None , u8"\xf0\x9e\xa4\xbc" }, /* 𞤼 : up=𞤚 : lo=𞤼 */
  { 0xf09ea4bd , 0xf09ea49b , 0xf09ea4bd , None , u8"\xf0\x9e\xa4\xbd" }, /* 𞤽 : up=𞤛 : lo=𞤽 */
  { 0xf09ea4be , 0xf09ea49c , 0xf09ea4be , None , u8"\xf0\x9e\xa4\xbe" }, /* 𞤾 : up=𞤜 : lo=𞤾 */
  { 0xf09ea4bf , 0xf09ea49d , 0xf09ea4bf , None , u8"\xf0\x9e\xa4\xbf" }, /* 𞤿 : up=𞤝 : lo=𞤿 */
};

}
