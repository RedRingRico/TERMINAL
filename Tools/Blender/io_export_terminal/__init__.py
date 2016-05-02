bl_info = {
	"name":			"Export [TERMINAL] model format",
	"author":		"Rico Tyrell",
	"blender":		( 2, 7, 0 ),
	"version":		( 0, 0, 0 ),
	"location":		"File > Import-Export",
	"description":	"Export as a [TERMINAL] model",
	"category":		"Import-Export"
}

import bpy;
import struct;
import io;
import os;
import codecs;
from bpy_extras.io_utils import ExportHelper

def b( x ):
	return codecs.ascii_encode( x )[ 0 ];

def menu_func( self, context ):
	self.layout.operator( Exporter.bl_idname, text="[TERMINAL] Model Format (.tml)" );

def register( ):
	bpy.utils.register_module( __name__ );
	bpy.types.INFO_MT_file_export.append( menu_func );

def unregister( ):
	bpy.utils.unregister_module( __name__ );
	bpy.types.INFO_MT_file_export.remove( menu_func );

if __name__ == "__main__":
	register( );

class Exporter( bpy.types.Operator, ExportHelper ):
	bl_idname		= "untitled.tml";
	bl_label		= "Export [TERMINAL]";
	bl_options		= { 'PRESET' };
	filename_ext	= ".tml";
	
	mesh_list		= [ ];
	skeleton_list	= [ ];
	
	def execute( self, context ):
		del self.mesh_list[ 0:len( self.mesh_list ) ];
		bpy.ops.object.mode_set( mode='OBJECT' );
		
		result = { 'FINISHED' };
		
		for Object in bpy.data.objects:
			if Object.type == 'MESH':
				self.mesh_list.append( Object );
			elif Object.type == 'ARMATURE':
				self.skeleton_list.append( Object );
		
		file = open( self.filepath, 'wb' );
		file.write( struct.pack( '<cccc', b'T', b'M', b'L', b'M' ) );
		file.write( struct.pack( '<cccc', b'T', b'E', b'S', b'T' ) );
		for x in range( 0, 28 ):
			file.write( struct.pack( "<c", b'\0' ) );
		file.write( struct.pack( "<I", len( self.mesh_list ) ) );
		for Mesh in self.mesh_list:
			WriteMeshChunks( file, Mesh );
		for Skeleton in self.skeleton_list:
			Skeleton.data.pose_position = 'REST';
			context.scene.update( );
			WriteSkeletonChunks( file, Skeleton );
			Skeleton.data.pose_position = 'POSE';
			context.scene.update( );
		WriteEndChunk( file );
		file.close( );
		
		return result;

def ExtractVertices( Mesh, VerticesOut, IndicesOut ):
	del VerticesOut[ : ];
	del IndicesOut[ : ];
	for Face in Mesh.data.polygons:
		Polygon = [ ];
		for Vertex, Loop in zip( Face.vertices, Face.loop_indices ):
			PackedVertex = [ Mesh.data.vertices[ Vertex ].co,
			Mesh.data.vertices[ Vertex ].normal,
			Mesh.data.uv_layers.active.data[ Loop ].uv ];
			VerticesOut.append( PackedVertex );
			Polygon.append( Loop );
		if( len( Polygon ) == 4 ):
			TempPolygon = Polygon.copy( );
			del Polygon[ : ];
			Polygon.append( TempPolygon[ 0 ] );
			Polygon.append( TempPolygon[ 1 ] );
			Polygon.append( TempPolygon[ 2 ] );
			Polygon.append( TempPolygon[ 0 ] );
			Polygon.append( TempPolygon[ 2 ] );
			Polygon.append( TempPolygon[ 3 ] );
		for Index in Polygon:
			IndicesOut.append( Index );

def WriteMeshChunks( File, Mesh ):
	File.write( struct.pack( "<I", 1 ) );
	File.write( struct.pack( "<I", 0 ) );
	FileSize = 0;
	# Flags
	FileSize += File.write( struct.pack( "<I", 2 ) );
	# Vertex count
	Vertices = [ ];
	Indices = [ ];
	ExtractVertices( Mesh, Vertices, Indices );
	FileSize += File.write( struct.pack( "<I", len( Vertices ) ) );
	# Material hash
	# There will need to be some way to set the material path per-mesh
	# and use the FNV1a hashing algorithm on it
	FileSize += File.write( struct.pack( "<I", 0 ) );
	# Lists
	FileSize += File.write( struct.pack( "<I", len( Indices ) ) );
	# Strips
	FileSize += File.write( struct.pack( "<I", 0 ) );
	# Fans
	FileSize += File.write( struct.pack( "<I", 0 ) );
	# Write out the vertices
	for Vertex in Vertices:
		# Get the world matrix to transform the vertices before exporting them
		TVertex = Mesh.matrix_world * Vertex[ 0 ];
		# Position
		FileSize += File.write( struct.pack( "<fff", TVertex[ 0 ], TVertex[ 2 ], TVertex[ 1 ] ) );
		# Normal
		FileSize += File.write( struct.pack( "<fff", Vertex[ 1 ][ 0 ], Vertex[ 1 ][ 2 ], Vertex[ 1 ][ 1 ] ) );
		# UV
		FileSize += File.write( struct.pack( "<ff", Vertex[ 2 ][ 0 ], 1.0 - Vertex[ 2 ][ 1 ] ) );
	# Write out the list indices
	for Index in Indices:
		FileSize += File.write( struct.pack( "<I", Index ) );
	File.seek( -( FileSize + 4 ), 1 );
	File.write( struct.pack( "<I", FileSize ) );
	File.seek( FileSize, 1 );
	print( "Mesh size: {}".format( FileSize ) );
	WriteEndChunk( File );

def WriteSkeletonChunks( File, Skeleton ):
	File.write( struct.pack( "<I", 4 ) );
	File.write( struct.pack( "<I", 0 ) );
	FileSize = 0;
	# Write out bone count
	JointCount = 0;
	FileSize += File.write( struct.pack( "B", JointCount ) );
	BoneNameMap = {};
	BoneIndex = 0;
	for Bone in Skeleton.data.bones:
		BoneNameMap[ Bone.name ] = BoneIndex;
		BoneIndex += 1;

	print( "Bone map: {}".format( BoneNameMap ) );

	for Bone in Skeleton.pose.bones:
		JointCount += 1;
		print( "Bone: ", format( Bone.name ) );
		print( "Head: {}".format( Bone.head ) );
		print( "Tail: {}".format( Bone.tail ) );
		print( "Orientation: {}".format( Bone.matrix.to_quaternion( ) ) );
		Flags = 0;
		FileSize += File.write( struct.pack( "B", len( Bone.name ) ) );
		FileSize += File.write( b( Bone.name ) )
		if( len( Bone.children ) > 0 ):
			print( "Children: {}".format( len( Bone.children ) ) );
		else:
			print( "No children" );
			Flags |= 0x00000001;
		FileSize += File.write( struct.pack( "<I", 0 ) );
		if Bone.parent is None:
			FileSize += File.write( struct.pack( "B", 255 ) );
			print( "Parent: None" );
		else:
			FileSize += File.write( struct.pack( "B", BoneNameMap[ Bone.parent.name ] ) );
			print( "Parent: {}".format( BoneNameMap[ Bone.parent.name ] ) );
		if( Bone.name == 'Root' ):
			Position = Skeleton.matrix_world * Bone.head;
		else:
			Position = Bone.head;
		FileSize += File.write( struct.pack( "<fff", Position[ 0 ], Position[ 2 ], -Position[ 1 ] ) );
		Quaternion = Bone.matrix.to_quaternion( );
		FileSize += File.write( struct.pack( "<ffff", Quaternion[ 0 ], Quaternion[ 1 ], Quaternion[ 2 ], Quaternion[ 3 ] ) );
		if( len( Bone.children ) == 0 ):
			JointCount += 1;
			FileSize += File.write( struct.pack( "B", 0 ) );
			FileSize += File.write( struct.pack( "<I", Flags ) );
			FileSize += File.write( struct.pack( "B", BoneNameMap[ Bone.name ] ) );
			Position = Bone.tail;
			FileSize += File.write( struct.pack( "<fff", Position[ 0 ], Position[ 2 ], -Position[ 1 ] ) );
			Quaternion = Bone.matrix.to_quaternion( );
			FileSize += File.write( struct.pack( "<ffff", Quaternion[ 0 ], Quaternion[ 1 ], Quaternion[ 2 ], Quaternion[ 3 ] ) );
	File.seek( -( FileSize + 4 ), 1 );
	File.write( struct.pack( "<I", FileSize ) );
	File.write( struct.pack( "B", JointCount ) );
	File.seek( FileSize - 1, 1 );
	WriteEndChunk( File );

def WriteEndChunk( File ):
	File.write( struct.pack( "<II", 0xFFFFFFFF, 0 ) );

