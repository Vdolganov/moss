from pathlib import Path
import re,sys
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'work/interface-v012/font-tools'))
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
glyphs=re.findall(r"case '([^']+)':\s*return \"([01]{35})\";",(root/'hardware lab/src/lab_ui.h').read_text())
patterns={ord(c):p for c,p in glyphs if len(c)==1}
patterns[32]='0'*35
for c in range(97,123):patterns[c]=patterns.get(c-32,'0'*35)
fb=FontBuilder(700,isTTF=True)
names=['.notdef']+[f'u{c}' for c in patterns]
fb.setupGlyphOrder(names);fb.setupCharacterMap({c:f'u{c}' for c in patterns})
out={};metrics={}
for name,p in [('.notdef','11111100011010110101101011000111111')]+[(f'u{c}',p) for c,p in patterns.items()]:
 pen=TTGlyphPen(None)
 for i,pixel in enumerate(p):
  if pixel=='1':
   x=(i%5)*100;y=(6-i//5)*100
   pen.moveTo((x,y));pen.lineTo((x+100,y));pen.lineTo((x+100,y+100));pen.lineTo((x,y+100));pen.closePath()
 out[name]=pen.glyph();metrics[name]=(600,0)
fb.setupGlyf(out);fb.setupHorizontalMetrics(metrics);fb.setupHorizontalHeader(ascent=700,descent=0)
fb.setupOS2(sTypoAscender=700,sTypoDescender=0,sTypoLineGap=0,usWinAscent=700,usWinDescent=0)
fb.setupNameTable({'familyName':'MOSS Bitmap','styleName':'Regular','uniqueFontIdentifier':'MOSSBitmap012','fullName':'MOSS Bitmap','psName':'MOSSBitmap'})
fb.setupPost();fb.setupMaxp();fb.font.flavor='woff2';fb.save(root/'visual prototype/current/assets/moss-bitmap.woff2')
