#!/bin/bash
mvn package
cp -f target/kurento-module-creator-jar-with-dependencies.jar $1/bin
cp -f scripts/kurento-module-creator $1/bin
chmod +x $1/bin/kurento-module-creator
cp target/classes/FindKurentoModuleCreator.cmake $1/share/cmake-2.8/Modules/
#target/classes/FindKurentoModuleCreator.cmake /usr/share/cmake-2.8/Modules/
#target/classes/FindKurentoModuleCreator.cmake /usr/share/cmake-3.0/Modules/
#target/classes/FindKurentoModuleCreator.cmake /usr/share/cmake-3.2/Modules/
#target/classes/FindKurentoModuleCreator.cmake /usr/share/cmake-3.5/Modules/

