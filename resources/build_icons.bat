del ..\RegionData.h
echo #pragma once >> ..\RegionData.h
python encode_regions.py Sun  Sun.bmp  >> ..\RegionData.h
python encode_regions.py Cloud  Cloud.bmp  >> ..\RegionData.h
python encode_regions.py Storm  Storm.bmp  >> ..\RegionData.h
