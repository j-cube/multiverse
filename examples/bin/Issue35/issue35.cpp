/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*                                                                           */
/*  Copyright 2015 J CUBE Inc. Tokyo, Japan.                                 */
/*                                                                           */
/*  Licensed under the Apache License, Version 2.0 (the "License");          */
/*  you may not use this file except in compliance with the License.         */
/*  You may obtain a copy of the License at                                  */
/*                                                                           */
/*      http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                           */
/*  Unless required by applicable law or agreed to in writing, software      */
/*  distributed under the License is distributed on an "AS IS" BASIS,        */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/*  See the License for the specific language governing permissions and      */
/*  limitations under the License.                                           */
/*****************************************************************************/

#include <cstdlib>
#include <cmath>

#include <vector>

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreGit/All.h>

void SetTimeSamples( Alembic::AbcGeom::OPolyMeshSchema & schema ,
                     int num_samples ,
                     std::vector< Imath::V3f > & points ,
                     std::vector< int > & face_countes ,
                     std::vector< int > & face_indices )
{
    for ( int i = 0 ; i < num_samples ; ++ i )
    {
        Alembic::AbcGeom::OPolyMeshSchema::Sample sample ;

        for ( int j = 1024 * 512 ; j < 1024 * 513 ; ++ j )
        {
            points[j] = Imath::V3f( j ) ;
        }

        Alembic::AbcGeom::P3fArraySample points_samples( & points[0] , points.size() ) ;
        sample.setPositions( points_samples ) ;

        Alembic::AbcGeom::Int32ArraySample face_counts_sample( & face_countes[0] , face_countes.size() ) ;
        sample.setFaceCounts( face_counts_sample ) ;

        Alembic::AbcGeom::Int32ArraySample face_indices_sample( & face_indices[0] , face_indices.size() ) ;
        sample.setFaceIndices( face_indices_sample ) ;

        schema.set( sample ) ;

        //
        std::cout << "Writing sample # " << i << std::endl ;
    }
}

int main( int argc , char * argv[] )
{
    // Generate random data for a mesh.
    //
    std::cout << ">> Generating random data..." << std::endl ;
    int num_points = 1024 * 1024 * 8 ;
    std::vector< Imath::V3f > points ;
    points.reserve( num_points ) ;
    for ( int i = 0 ; i < num_points ; ++ i )
    {
        const Imath::V3f & point = Imath::V3f( rand() / static_cast< float >( RAND_MAX ) ,
                                               rand() / static_cast< float >( RAND_MAX ) ,
                                               rand() / static_cast< float >( RAND_MAX ) ) ;
        points.push_back( point ) ;
    }

    std::vector< int > faces( 1024 * 32 , 4 ) ;

    int num_indices = 1024 * 32 * 4 ;
    std::vector< int > indices ;
    indices.reserve( num_indices ) ;
    for ( int i = 0 ; i < num_indices ; ++ i )
    {
        indices.push_back( i ) ;
        indices.push_back( i + 1 ) ;
        indices.push_back( i + 2 ) ;
        indices.push_back( i + 3 ) ;
    }

    int num_samples = 200 ;

    // Write the num_samples by Git.
    //
    int x ;
    std::cout << ">> enter integer: ";
    std::cin >> x ;

    std::cout << ">> Start to write..." << std::endl ;
    {
        Alembic::Abc::OArchive ar = Alembic::Abc::CreateArchiveWithInfo( Alembic::AbcCoreGit::WriteArchive() ,
                                                                            "DelayedWriting-git.abc" ,
                                                                            "DelayedWriting" ,
                                                                            "" ) ;
        Alembic::AbcGeom::OPolyMesh mesh( ar.getTop() , "mesh" ) ;
        Alembic::AbcGeom::OPolyMeshSchema schema = mesh.getSchema() ;
        SetTimeSamples( schema , num_samples , points , faces , indices ) ;
    }
    std::cout << ">> End" << std::endl ;


    std::cout << ">> enter integer: ";
    std::cin >> x ;


    return EXIT_SUCCESS ;
}
