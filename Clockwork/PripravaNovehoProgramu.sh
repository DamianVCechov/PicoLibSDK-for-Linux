#!/bin/bash

read -p "Zadejte jméno nového projektu: " GRPDIR
read -p "Zadejte jméno nového programu: " TARGET

mkdir -p ${GRPDIR}/${TARGET}
cd ${GRPDIR}/${TARGET}

echo -e '#!/bin/bash\n\n# All Re-compilation\n\nbash d.sh\nbash c.sh\nbash e.sh' > a.sh
echo -e '#!/bin/bash\n\n# Compilation...\n\nexport TARGET='${TARGET}'\nexport GRPDIR='${GRPDIR}'\n\n../../../_c1.sh "$1"' > c.sh
echo -e '#!/bin/bash\n\n# Delete...\n\nexport TARGET='${TARGET}'\n\n../../../_d1.sh' > d.sh
echo -e '#!/bin/bash\n\n# Export to hardware\n\nexport TARGET='${TARGET}'\n\n../../../_e1.sh' > e.sh
mkdir src
echo -e '#!/bin/bash\n\ncd ..\n./c.sh\ncd src' > src/c.sh
echo -e '#!/bin/bash\n\ncd ..\n./c.sh\n./e.sh\ncd src' > src/ce.sh
echo -e '#!/bin/bash\n\ncd ..\n./d.sh\ncd src' > src/d.sh
echo -e '#!/bin/bash\n\ncd ..\n./e.sh\ncd src' > src/e.sh

touch src/main.h
touch src/main.cpp

echo -e '#include "../../../includes.h"\n#include "src/main.h"' > include.h
echo -e '#ifndef _CONFIG_H\n#define _CONFIG_H\n\n\n#include "../../../config_def.h"\n#endif' > config.h
echo -e '# ASM source files\nASRC +=\n\n# C source files\nCSRC +=\n\n# C++ source files\nSRC += src/main.cpp\n\n# Makefile includes\ninclude ../../../Makefile.inc' > Makefile

find . -name '*.sh' -exec chmod +x {} +

echo "OK :)"
