/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2015 Blender Foundation.
 * All rights reserved.
 *
 * Contributor(s): Kevin Dietrich
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "openvdb_capi.h"
#include "openvdb_dense_convert.h"
#include "openvdb_util.h"

using namespace openvdb;

struct OpenVDBFloatGrid { int unused; };
struct OpenVDBIntGrid { int unused; };
struct OpenVDBVectorGrid { int unused; };

int OpenVDB_getVersionHex()
{
    return OPENVDB_LIBRARY_VERSION;
}

void OpenVDB_get_grid_names_and_types(const char *filename,
                                      char **grid_names,
                                      char **grid_types,
                                      int *num_grids)
{
	int ret = OPENVDB_NO_ERROR;
	initialize();

	try {
		io::File file(filename);
		file.open();

		GridPtrVecPtr grids = file.getGrids();

		*num_grids = grids->size();

		for (size_t i = 0; i < *num_grids; ++i) {
			GridBase::ConstPtr grid = (*grids)[i];
			grid_names[i] = strdup(grid->getName().c_str());

			/* XXX - this is blender specific, for external files it might crash if
			 * it doens't find a "is_color" metaValue, cases where it doens't crash
			 * are when the vector grids are at the end of the grid vector.
			 */
			if (grid->valueType() == "vec3s" && grid->metaValue<bool>("is_color")) {
				grid_types[i] = strdup("color");
			}
			else {
				grid_types[i] = strdup(grid->valueType().c_str());
			}
		}
	}
	catch (...) {
		catch_exception(ret);
	}
}

void OpenVDB_update_fluid_transform(const char *filename,
                                    float matrix[4][4],
                                    float matrix_high[4][4])
{
	int ret = OPENVDB_NO_ERROR;

	try {
		internal::OpenVDB_update_fluid_transform(filename, matrix, matrix_high);
	}
	catch (...) {
		catch_exception(ret);
	}
}

OpenVDBFloatGrid *OpenVDB_export_grid_fl(OpenVDBWriter *writer,
                                         const char *name, float *data,
                                         const int res[3], float matrix[4][4],
                                         OpenVDBFloatGrid *mask)
{
	OpenVDBFloatGrid *grid =
	        (OpenVDBFloatGrid *)internal::OpenVDB_export_grid<FloatGrid>(writer, name, data, res, matrix, (FloatGrid *)mask);
	return grid;
}

OpenVDBIntGrid *OpenVDB_export_grid_ch(OpenVDBWriter *writer,
                                       const char *name, unsigned char *data,
                                       const int res[3], float matrix[4][4],
                                       OpenVDBFloatGrid *mask)
{
	OpenVDBIntGrid *grid =
	        (OpenVDBIntGrid *)internal::OpenVDB_export_grid<Int32Grid>(writer, name, data, res, matrix, (FloatGrid *)mask);
	return grid;
}

OpenVDBVectorGrid *OpenVDB_export_grid_vec(struct OpenVDBWriter *writer,
                                           const char *name,
                                           const float *data_x, const float *data_y, const float *data_z,
                                           const int res[3], float matrix[4][4], short vec_type,
                                           const bool is_color, OpenVDBFloatGrid *mask)
{
	OpenVDBVectorGrid *grid =
	(OpenVDBVectorGrid *)internal::OpenVDB_export_vector_grid(writer, name,
	                                     data_x, data_y, data_z, res, matrix,
	                                     static_cast<openvdb::VecType>(vec_type),
	                                     is_color, (FloatGrid *)mask);
	return grid;
}

void OpenVDB_import_grid_fl(OpenVDBReader *reader,
                            const char *name, float **data,
                            const int res[3])
{
	internal::OpenVDB_import_grid<FloatGrid>(reader, name, data, res);
}

void OpenVDB_import_grid_ch(OpenVDBReader *reader,
                            const char *name, unsigned char **data,
                            const int res[3])
{
	internal::OpenVDB_import_grid<Int32Grid>(reader, name, data, res);
}

void OpenVDB_import_grid_vec(struct OpenVDBReader *reader,
                             const char *name,
                             float **data_x, float **data_y, float **data_z,
                             const int res[3])
{
	internal::OpenVDB_import_grid_vector(reader, name, data_x, data_y, data_z, res);
}

OpenVDBWriter *OpenVDBWriter_create()
{
	return new OpenVDBWriter();
}

void OpenVDBWriter_free(OpenVDBWriter *writer)
{
	delete writer;
	writer = NULL;
}

void OpenVDBWriter_set_compression(OpenVDBWriter *writer, const int flag)
{
	int compression_flags = io::COMPRESS_ACTIVE_MASK;

	if (flag == 0) {
		compression_flags |= io::COMPRESS_ZIP;
	}
	else if (flag == 1) {
		compression_flags |= io::COMPRESS_BLOSC;
	}
	else {
		compression_flags = io::COMPRESS_NONE;
	}

	writer->setFileCompression(compression_flags);
}

void OpenVDBWriter_add_meta_fl(OpenVDBWriter *writer, const char *name, const float value)
{
	writer->insertFloatMeta(name, value);
}

void OpenVDBWriter_add_meta_int(OpenVDBWriter *writer, const char *name, const int value)
{
	writer->insertIntMeta(name, value);
}

void OpenVDBWriter_add_meta_v3(OpenVDBWriter *writer, const char *name, const float value[3])
{
	writer->insertVec3sMeta(name, value);
}

void OpenVDBWriter_add_meta_v3_int(OpenVDBWriter *writer, const char *name, const int value[3])
{
	writer->insertVec3IMeta(name, value);
}

void OpenVDBWriter_add_meta_mat4(OpenVDBWriter *writer, const char *name, float value[4][4])
{
	writer->insertMat4sMeta(name, value);
}

void OpenVDBWriter_write(OpenVDBWriter *writer, const char *filename)
{
	writer->write(filename);
}

OpenVDBReader *OpenVDBReader_create()
{
	return new OpenVDBReader();
}

void OpenVDBReader_free(OpenVDBReader *reader)
{
	delete reader;
	reader = NULL;
}

void OpenVDBReader_open(OpenVDBReader *reader, const char *filename)
{
	reader->open(filename);
}

void OpenVDBReader_get_meta_fl(OpenVDBReader *reader, const char *name, float *value)
{
	reader->floatMeta(name, *value);
}

void OpenVDBReader_get_meta_int(OpenVDBReader *reader, const char *name, int *value)
{
	reader->intMeta(name, *value);
}

void OpenVDBReader_get_meta_v3(OpenVDBReader *reader, const char *name, float value[3])
{
	reader->vec3sMeta(name, value);
}

void OpenVDBReader_get_meta_v3_int(OpenVDBReader *reader, const char *name, int value[3])
{
	reader->vec3IMeta(name, value);
}

void OpenVDBReader_get_meta_mat4(OpenVDBReader *reader, const char *name, float value[4][4])
{
	reader->mat4sMeta(name, value);
}
