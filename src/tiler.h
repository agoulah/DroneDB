/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef TILER_H
#define TILER_H

#include <gdal_priv.h>
#include <ogr_srs_api.h>
#include <gdalwarper.h>
#include <sstream>
#include <string>
#include "geo.h"
#include "fs.h"

namespace ddb{


class GlobalMercator{
    double originShift;
    double initialResolution;
    int maxZoomLevel;
    int tileSize;
public:
    GlobalMercator();

    BoundingBox<Geographic2D> tileLatLonBounds(int tx, int ty, int zoom) const;

    // Bounds of the given tile in EPSG:3857 coordinates
    BoundingBox<Projected2D> tileBounds(int tx, int ty, int zoom) const;

    // Converts XY point from Spherical Mercator EPSG:3857 to lat/lon in WGS84 Datum
    Geographic2D metersToLatLon(int mx, int my) const;

    // Tile for given mercator coordinates
    Projected2D metersToTile(int mx, int my, int zoom) const;

    // Converts pixel coordinates in given zoom level of pyramid to EPSG:3857"
    Projected2D pixelsToMeters(int px, int py, int zoom) const;

    // Converts EPSG:3857 to pyramid pixel coordinates in given zoom level
    Projected2D metersToPixels(int mx, int my, int zoom) const;

    // Tile covering region in given pixel coordinates
    Projected2D pixelsToTile(int px, int py) const;

    // Resolution (meters/pixel) for given zoom level (measured at Equator)
    double resolution(int zoom) const;

    // Maximal scaledown zoom of the pyramid closest to the pixelSize.
    int zoomForPixelSize(double pixelSize) const;
};

struct GeoExtent{
    int x;
    int y;
    int xsize;
    int ysize;
};

struct GQResult{
    GeoExtent r;
    GeoExtent w;
};

struct TileInfo{
    int tx, ty, tz;
    TileInfo(int tx, int ty, int tz) :
        tx(tx), ty(ty), tz(tz){};
};

class Tiler{
    int tileSize;
    std::string geotiffPath;
    fs::path outputFolder;
    bool tms;

    GDALDriverH pngDrv;
    GDALDriverH memDrv;

    GDALDatasetH inputDataset;
    int rasterCount;
    int nBands;

    double oMinX, oMaxX, oMaxY, oMinY;
    GlobalMercator mercator;
    int tMaxZ;
    int tMinZ;


    bool hasGeoreference(const GDALDatasetH &dataset);
    bool sameProjection(const OGRSpatialReferenceH &a, const OGRSpatialReferenceH &b);

    int dataBandsCount(const GDALDatasetH &dataset);
    std::string getTilePath(int z, int x, int y, bool createIfNotExists);
    GDALDatasetH createWarpedVRT(const GDALDatasetH &src, const OGRSpatialReferenceH &srs, GDALResampleAlg resampling =  GRA_NearestNeighbour);

    // Returns parameters reading raster data.
    // (coordinates and x/y shifts for border tiles).
    // If the querysize is not given, the
    // extent is returned in the native resolution of dataset ds.
    GQResult geoQuery(GDALDatasetH ds, double ulx, double uly, double lrx, double lry, int querySize = 0);

    // Convert from TMS to XYZ
    int tmsToXYZ(int ty, int tz) const;

    // Convert from XYZ to TMS
    int xyzToTMS(int ty, int tz) const;

    template <typename T>
    void rescale(GDALRasterBandH hBand, char *buffer, size_t bufsize);
public:
    Tiler(const std::string &geotiffPath, const std::string &outputFolder, bool tms = false);
    ~Tiler();

    std::string tile(int tz, int tx, int ty);
    std::string tile(const TileInfo &tile);

    std::vector<TileInfo> getTilesForZoomLevel(int tz) const;
    BoundingBox<int> getMinMaxZ() const;

    // Min max tile coordinates for specified zoom level
    BoundingBox<Projected2D> getMinMaxCoordsForZ(int tz) const;

};

class TilerHelper{
    // Parses a string (either "N" or "min-max") and returns
    // two ints (min,max)
    static BoundingBox<int> parseZRange(const std::string &zRange);

public:
    static void runTiler(Tiler &tiler, std::ostream &output = std::cout, const std::string &format = "text", const std::string &zRange = "auto", const std::string &x = "auto", const std::string &y = "auto");
};

}

#endif // TILER_H
