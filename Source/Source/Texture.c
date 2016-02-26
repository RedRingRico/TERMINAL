#include <Texture.h>
#include <FileSystem.h>
#include <Log.h>

typedef struct _tagGLOBAL_INDEX
{
	char	ID[ 4 ];
	Uint32	Size;
}GLOBAL_INDEX,*PGLOBAL_INDEX;

typedef struct _tagPVRT_HEADER
{
	char	ID[ 4 ];
	Uint32	Size;
	Uint32	Flags;
	Uint16	Width;
	Uint16	Height;
}PVRT_HEADER,*PPVRT_HEADER;

int TEX_LoadTexture( PTEXTURE p_pTexture, const char *p_pFileName )
{
	GDFS FileHandle;
	long FileBlocks;
	PKMDWORD pTexture;
	GLOBAL_INDEX GlobalIndex;
	PVRT_HEADER PVRTHeader;
	Sint32 TextureOffset = 0;
	Sint32 FileSize = 0;
	Sint32 DataOffset = 0;

	if( !( FileHandle = FS_OpenFile( p_pFileName ) ) )
	{
		LOG_Debug( "Failed to open texture: %s", p_pFileName );

		return 1;
	}

	gdFsGetFileSize( FileHandle, &FileSize );
	gdFsGetFileSctSize( FileHandle, &FileBlocks );

	pTexture = syMalloc( FileBlocks * 2048 );

	gdFsReqRd32( FileHandle, FileBlocks, pTexture );
	
	while( gdFsGetStat( FileHandle ) != GDD_STAT_COMPLETE )
	{
	}

	gdFsClose( FileHandle );

	FileSize /= 4;

	while( TextureOffset < FileSize )
	{
		/* GBIX */
		if( pTexture[ TextureOffset ] == 0x58494247 )
		{
			memcpy( &GlobalIndex, &pTexture[ TextureOffset ],
				sizeof( GlobalIndex ) );
			DataOffset += ( sizeof( GLOBAL_INDEX ) + GlobalIndex.Size ) / 4;
			TextureOffset += ( sizeof( GLOBAL_INDEX ) + GlobalIndex.Size ) / 4;
		}

		/* PVRT */
		if( pTexture[ TextureOffset ] == 0x54525650 )
		{
			memcpy( &PVRTHeader, &pTexture[ TextureOffset / 4 ],
				sizeof( PVRTHeader ) );

			DataOffset += sizeof( PVRT_HEADER ) / 4;
			TextureOffset +=
				( ( sizeof( Uint32 ) * 2 ) + PVRTHeader.Size ) / 4;

			p_pTexture->Width = PVRTHeader.Width;
			p_pTexture->Height = PVRTHeader.Height;
			p_pTexture->Flags = PVRTHeader.Flags;
		}
	}

	kmCreateTextureSurface( &p_pTexture->SurfaceDescription,
		p_pTexture->Width, p_pTexture->Height, p_pTexture->Flags );

	kmLoadTexture( &p_pTexture->SurfaceDescription, pTexture + DataOffset );

	while( kmQueryFinishLastTextureDMA( ) != KMSTATUS_SUCCESS )
	{
	}

	syFree( pTexture );

	return 0;
}

void TEX_DeleteTexture( PTEXTURE p_pTexture )
{
	kmFreeTexture( &p_pTexture->SurfaceDescription );
}

