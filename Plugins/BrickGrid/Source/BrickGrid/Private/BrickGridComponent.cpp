// Copyright 2014, Andrew Scheidecker. All Rights Reserved.

#include "BrickGridPluginPrivatePCH.h"
#include "ComponentReregisterContext.h"
#include "BrickRenderComponent.h"
#include "BrickCollisionComponent.h"
#include "BrickGridComponent.h"




// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.

FInt3 NeighborsCoordinates[8] =
{
	//X,Y,Z
	FInt3(-1, +1, 0),//-XY	1
	FInt3(0, +1, 0),//Y		2
	FInt3(+1, +1, 0),//XY	3
	FInt3(+1, 0, 0),//X		4
	FInt3(+1, -1, 0),//X-Y	5
	FInt3(0, -1, 0),//-Y	6
	FInt3(-1, -1, 0),//-X-Y 7
	FInt3(-1, 0, 0)	//-X	8
	//it emulates clockwise rotation.

};


void UBrickGridComponent::Init(const FBrickGridParameters& InParameters)
{
	Parameters = InParameters;

	// Validate the empty material index.
	Parameters.EmptyMaterialIndex = FMath::Clamp<int32>(Parameters.EmptyMaterialIndex, 0, Parameters.Materials.Num() - 1);

	// Limit each region to 128x128x128 bricks, which is the largest power of 2 size that can be rendered using 8-bit relative vertex positions.
	Parameters.BricksPerRegionLog2 = FInt3::Clamp(Parameters.BricksPerRegionLog2, FInt3::Scalar(0), FInt3::Scalar(BrickGridConstants::MaxBricksPerRegionAxisLog2));

	// Don't allow fractional chunks/region, or chunks smaller than one brick.
	Parameters.RenderChunksPerRegionLog2 = FInt3::Clamp(Parameters.RenderChunksPerRegionLog2, FInt3::Scalar(0), Parameters.BricksPerRegionLog2);
	Parameters.CollisionChunksPerRegionLog2 = FInt3::Clamp(Parameters.CollisionChunksPerRegionLog2, FInt3::Scalar(0), Parameters.BricksPerRegionLog2);

	// Derive the chunk and region sizes from the log2 inputs.
	BricksPerRenderChunkLog2 = Parameters.BricksPerRegionLog2 - Parameters.RenderChunksPerRegionLog2;
	BricksPerCollisionChunkLog2 = Parameters.BricksPerRegionLog2 - Parameters.CollisionChunksPerRegionLog2;
	BricksPerRegion = BricksPerCollisionChunk = FInt3::Exp2(Parameters.BricksPerRegionLog2);
	BricksPerRenderChunk = FInt3::Exp2(BricksPerRenderChunkLog2);
	BricksPerCollisionChunk = FInt3::Exp2(BricksPerCollisionChunkLog2);
	RenderChunksPerRegion = FInt3::Exp2(Parameters.RenderChunksPerRegionLog2);
	CollisionChunksPerRegion = FInt3::Exp2(Parameters.CollisionChunksPerRegionLog2);
	BricksPerRegion = FInt3::Exp2(Parameters.BricksPerRegionLog2);

	// Clamp the min/max region coordinates to keep brick coordinates within 32-bit signed integers.
	Parameters.MinRegionCoordinates = FInt3::Max(Parameters.MinRegionCoordinates, FInt3::Scalar(INT_MIN) / BricksPerRegion);
	Parameters.MaxRegionCoordinates = FInt3::Min(Parameters.MaxRegionCoordinates, (FInt3::Scalar(INT_MAX) - BricksPerRegion + FInt3::Scalar(1)) / BricksPerRegion);
	MinBrickCoordinates = Parameters.MinRegionCoordinates * BricksPerRegion;
	MaxBrickCoordinates = Parameters.MaxRegionCoordinates * BricksPerRegion + BricksPerRegion - FInt3::Scalar(1);

	// Limit the ambient occlusion blur radius to be a positive value.
	Parameters.AmbientOcclusionBlurRadius = FMath::Max(0, Parameters.AmbientOcclusionBlurRadius);

	// Reset the regions and reregister the component.
	FComponentReregisterContext ReregisterContext(this);
	Regions.Empty();
	RegionCoordinatesToIndex.Empty();
	for (auto ChunkIt = RenderChunkCoordinatesToComponent.CreateConstIterator(); ChunkIt; ++ChunkIt)
	{
		ChunkIt.Value()->DetachFromParent();
		ChunkIt.Value()->DestroyComponent();
	}
	for (auto ChunkIt = CollisionChunkCoordinatesToComponent.CreateConstIterator(); ChunkIt; ++ChunkIt)
	{
		ChunkIt.Value()->DetachFromParent();
		ChunkIt.Value()->DestroyComponent();
	}
	RenderChunkCoordinatesToComponent.Empty();
	CollisionChunkCoordinatesToComponent.Empty();
}


bool UBrickGridComponent::ReadRegionUncompressed(FBrickRegion& RegionToRead)
{


	std::ostringstream X, Y, Z;
	X << RegionToRead.Coordinates.X;
	Y << RegionToRead.Coordinates.Y;
	Z << RegionToRead.Coordinates.Z;
	FString title;
	title = "C:/Users/Migue/Desktop/Regions/";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();
	title += ".bin";




	TArray<uint8>UncompressedBinaryArray;

	UncompressedBinaryArray.SetNum((1 << (Parameters.BricksPerRegionLog2.X + Parameters.BricksPerRegionLog2.Y)) * 64 * 2);

	FFileHelper::LoadFileToArray(UncompressedBinaryArray, *title);
	RegionToRead.BrickContents = UncompressedBinaryArray;
	//CalculatePosibleWaterSurfacesAndRespectiveVolumesAndFlux(RegionToRead);
	ReadRegionLakes(RegionToRead);
	UpdateMaxNonEmptyBrickMap(RegionToRead, FInt3::Scalar(0), BricksPerRegion - FInt3::Scalar(1));
	return true;
}
void UBrickGridComponent::CalculatePosibleWaterSurfacesAndRespectiveVolumesAndFlux(FBrickRegion& RegionToRead)
{
	TQueue<FInt3> QueueOfLakesCoordinates;
	TArray<bool>VisitedBrickIndexes;
	VisitedBrickIndexes.SetNumUninitialized(BricksPerRegion.X * BricksPerRegion.Y * BricksPerRegion.Z);
	for (int32 iterator = 0; iterator < VisitedBrickIndexes.Num(); iterator++)
		VisitedBrickIndexes[iterator] = false;
	TArray<int32>DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData;
	DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData.SetNumUninitialized(BricksPerRegion.X * BricksPerRegion.Y * BricksPerRegion.Z);
	for (int32 iterator = 0; iterator < DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData.Num(); iterator++)
		DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData[iterator] = -1;

	FBrickRegion::LakeSlice LakeSlice;
	for (int32 LocalZ = 0; LocalZ < BricksPerRegion.Z; ++LocalZ)
	{
		for (int32 LocalX = 0; LocalX < BricksPerRegion.X; ++LocalX)
		{
			for (int32 LocalY = 0; LocalY < BricksPerRegion.Y; ++LocalY)
			{
				int32 FirstIndex = ((LocalY * BricksPerRegion.X) + LocalX) * BricksPerRegion.Z + LocalZ;// is always less than 131072
				FInt3 Coordinates(LocalX, LocalY, LocalZ);
				if (RegionToRead.BrickContents[FirstIndex] == 0 && !VisitedBrickIndexes[FirstIndex])
				{

					VisitedBrickIndexes[FirstIndex] = true;
					LakeSlice.Index = FirstIndex;
					LakeSlice.Coordinates = Coordinates;
					QueueOfLakesCoordinates.Enqueue(Coordinates);
					bool IndexIsOnRegionFrontier = true;
					if (LocalX == 0)
					{
						LakeSlice.BricksOnTheRegionFrontierAtMinusX.Add(FirstIndex);
					}
					if (LocalY == 0)
					{
						LakeSlice.BricksOnTheRegionFrontierAtMinusY.Add(FirstIndex);
					}
					if (LocalX >= BricksPerRegion.X - 1)
					{
						LakeSlice.BricksOnTheRegionFrontierAtX.Add(FirstIndex);
					}
					if (LocalY >= BricksPerRegion.Y - 1)
					{
						LakeSlice.BricksOnTheRegionFrontierAtY.Add(FirstIndex);
					}

					IndexIsOnRegionFrontier = false;
					FInt3 LakeCoordinates(LocalX, LocalY, LocalZ);
					while (!QueueOfLakesCoordinates.IsEmpty())
					{
						QueueOfLakesCoordinates.Dequeue(LakeCoordinates);
						int32 BrickIndex = ((LakeCoordinates.Y * BricksPerRegion.X) + LakeCoordinates.X) * BricksPerRegion.Z + LakeCoordinates.Z;
						DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData[BrickIndex] = LakeSlice.Index;
						LakeSlice.Volume += 1;
						LakeSlice.LakeBricks.Add(BrickIndex);
						FInt3 NeighboorBrickAtX(1, 0, 0);
						FInt3 NeighboorBrickAtMinusX(-1, 0, 0);
						FInt3 NeighboorBrickAtY(0, 1, 0);
						FInt3 NeighboorBrickAtMinusY(0, -1, 0);
						NeighboorBrickAtX = NeighboorBrickAtX + LakeCoordinates;
						NeighboorBrickAtMinusX = NeighboorBrickAtMinusX + LakeCoordinates;
						NeighboorBrickAtY = NeighboorBrickAtY + LakeCoordinates;
						NeighboorBrickAtMinusY = NeighboorBrickAtMinusY + LakeCoordinates;

						int32 NeighboorBrickAtXIndex = ((NeighboorBrickAtX.Y * BricksPerRegion.X) + NeighboorBrickAtX.X) * BricksPerRegion.Z + NeighboorBrickAtX.Z;// is always less than 131072
						int32 NeighboorBrickAtMinusXIndex = ((NeighboorBrickAtMinusX.Y * BricksPerRegion.X) + NeighboorBrickAtMinusX.X) * BricksPerRegion.Z + NeighboorBrickAtMinusX.Z;// is always less than 131072
						int32 NeighboorBrickAtYIndex = ((NeighboorBrickAtY.Y * BricksPerRegion.X) + NeighboorBrickAtY.X) * BricksPerRegion.Z + NeighboorBrickAtY.Z;// is always less than 131072
						int32 NeighboorBrickAtMinusYIndex = ((NeighboorBrickAtMinusY.Y * BricksPerRegion.X) + NeighboorBrickAtMinusY.X) * BricksPerRegion.Z + NeighboorBrickAtMinusY.Z;// is always less than 131072
						if (LakeCoordinates.X < BricksPerRegion.X - 1)
						{
							if (!VisitedBrickIndexes[NeighboorBrickAtXIndex])
							{
								VisitedBrickIndexes[NeighboorBrickAtXIndex] = true;
								if (RegionToRead.BrickContents[NeighboorBrickAtXIndex] == 0)
								{
									QueueOfLakesCoordinates.Enqueue(NeighboorBrickAtX);
								}
							}
						}
						else
							LakeSlice.BricksOnTheRegionFrontierAtX.Add(BrickIndex);
						if (LakeCoordinates.X > 0 + 1)
						{
							if (!VisitedBrickIndexes[NeighboorBrickAtMinusXIndex])
							{
								VisitedBrickIndexes[NeighboorBrickAtMinusXIndex] = true;
								if (RegionToRead.BrickContents[NeighboorBrickAtMinusXIndex] == 0)
								{
									QueueOfLakesCoordinates.Enqueue(NeighboorBrickAtMinusX);
								}
							}
						}
						else if (LakeCoordinates.X == 0)
							LakeSlice.BricksOnTheRegionFrontierAtMinusX.Add(BrickIndex);
						if (LakeCoordinates.Y < BricksPerRegion.Y - 1)
						{
							if (!VisitedBrickIndexes[NeighboorBrickAtYIndex])
							{
								VisitedBrickIndexes[NeighboorBrickAtYIndex] = true;
								if (RegionToRead.BrickContents[NeighboorBrickAtYIndex] == 0)
								{

									QueueOfLakesCoordinates.Enqueue(NeighboorBrickAtY);
								}
							}
						}
						else
							LakeSlice.BricksOnTheRegionFrontierAtY.Add(BrickIndex);
						if (LakeCoordinates.Y > 0 + 1)
						{
							if (!VisitedBrickIndexes[NeighboorBrickAtMinusYIndex])
							{
								VisitedBrickIndexes[NeighboorBrickAtMinusYIndex] = true;
								if (RegionToRead.BrickContents[NeighboorBrickAtMinusYIndex] == 0)
								{
									QueueOfLakesCoordinates.Enqueue(NeighboorBrickAtMinusY);
								}
							}
						}
						else if (LakeCoordinates.Y == 0)
							LakeSlice.BricksOnTheRegionFrontierAtMinusY.Add(BrickIndex);
					}// END OF BFS


					FromBrickCoordinatesSaveRegionLake(RegionToRead, LakeSlice);
					FBrickRegion::LakeSlice EmptyLake;
					LakeSlice = EmptyLake;
				}//END OF LAKESLICE CONFIGURATION
			}
		}
	}// END of the 3 FOR LOOPS
	RegionToRead.DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData = DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData;
	SaveRegionLakes(RegionToRead);
}
void UBrickGridComponent::SaveRegionLakes(FBrickRegion& RegionToRead)
{
	std::ostringstream X, Y, Z;
	X << RegionToRead.Coordinates.X;
	Y << RegionToRead.Coordinates.Y;
	Z << RegionToRead.Coordinates.Z;
	FString title;
	title = "C:/Users/Migue/Desktop/Regions/";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();
	title += "/";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();
	title += ".txt";
	FString array;
	for (int iterator = 0; iterator < RegionToRead.DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData.Num(); iterator++)
		array += FString::FromInt(RegionToRead.DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData[iterator]) + "\r\n";
	FFileHelper::SaveStringToFile(array, *title);
}
void UBrickGridComponent::FromBrickCoordinatesSaveRegionLake(FBrickRegion& RegionToRead, FBrickRegion::LakeSlice &LakeSliceToSave)
{
	std::ostringstream X, Y, Z;
	X << RegionToRead.Coordinates.X;
	Y << RegionToRead.Coordinates.Y;
	Z << RegionToRead.Coordinates.Z;
	std::ostringstream LakeSliceToSaveIndex;
	LakeSliceToSaveIndex << LakeSliceToSave.Index;
	FString title;
	title = "C:/Users/Migue/Desktop/Regions/";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();
	title += "/";
	title += (LakeSliceToSaveIndex.str()).c_str();
	title += ".txt";
	FString FString;
	FString += FString::FromInt(LakeSliceToSave.Index) + "\r\n";
	FString += FString::FromInt(LakeSliceToSave.Volume) + "\r\n";
	FString += FString::FromInt(LakeSliceToSave.Coordinates.X) + "\r\n";
	FString += FString::FromInt(LakeSliceToSave.Coordinates.Y) + "\r\n";
	FString += FString::FromInt(LakeSliceToSave.Coordinates.Z) + "\r\n";
	for (int iterator = 0; iterator < LakeSliceToSave.LakeBricks.Num(); iterator++)
	{
		FString += FString::FromInt(LakeSliceToSave.LakeBricks[iterator]) + "\r\n";
	}
	FString += "BricksOnTheRegionFrontierAtX\r\n";
	for (int iterator = 0; iterator < LakeSliceToSave.BricksOnTheRegionFrontierAtX.Num(); iterator++)
	{
		FString += FString::FromInt(LakeSliceToSave.BricksOnTheRegionFrontierAtX[iterator]) + "\r\n";
	}
	FString += "BricksOnTheRegionFrontierAtMinusX\r\n";
	for (int iterator = 0; iterator < LakeSliceToSave.BricksOnTheRegionFrontierAtMinusX.Num(); iterator++)
	{
		FString += FString::FromInt(LakeSliceToSave.BricksOnTheRegionFrontierAtMinusX[iterator]) + "\r\n";
	}
	FString += "BricksOnTheRegionFrontierAtY\r\n";
	for (int iterator = 0; iterator < LakeSliceToSave.BricksOnTheRegionFrontierAtY.Num(); iterator++)
	{
		FString += FString::FromInt(LakeSliceToSave.BricksOnTheRegionFrontierAtY[iterator]) + "\r\n";
	}
	FString += "BricksOnTheRegionFrontierAtMinusY\r\n";
	for (int iterator = 0; iterator < LakeSliceToSave.BricksOnTheRegionFrontierAtMinusY.Num(); iterator++)
	{
		FString += FString::FromInt(LakeSliceToSave.BricksOnTheRegionFrontierAtMinusY[iterator]) + "\r\n";
	}
	FFileHelper::SaveStringToFile(FString, *title);

}
void UBrickGridComponent::ReadRegionLakes(FBrickRegion& RegionToRead)
{
	std::ostringstream X, Y, Z;
	X << RegionToRead.Coordinates.X;
	Y << RegionToRead.Coordinates.Y;
	Z << RegionToRead.Coordinates.Z;
	FString title;
	title = "C:/Users/Migue/Desktop/Regions/";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();
	title += "/";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();
	title += ".txt";
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*title))
	{
		//CalculatePosibleWaterSurfacesAndRespectiveVolumesAndFlux(RegionToRead);
	}
	else
	{
		TArray <FString> lines;
		FFileHelper::LoadANSITextFileToStrings(*title, NULL, lines);
		for (int iterator = 0; iterator < lines.Num(); iterator++)
		{
			RegionToRead.DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData.Add(FCString::Atoi(*lines[iterator]));
		}
	}
}
bool UBrickGridComponent::FromBrickCoordinatesFindRegionLake(FBrickRegion& RegionToRead, int &BrickCoordinates, FBrickRegion::LakeSlice &LakeSliceToRead)
{
	if (RegionToRead.DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData.Num() == 0)
	{
		return false;
	}
	else
	{
		int LakeIndex = RegionToRead.DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData[BrickCoordinates];
		if (LakeIndex != 0)//CHANGE 0 to -1
		{
			std::ostringstream X, Y, Z;
			X << RegionToRead.Coordinates.X;
			Y << RegionToRead.Coordinates.Y;
			Z << RegionToRead.Coordinates.Z;
			FString title;
			title = "C:/Users/Migue/Desktop/Regions/";
			title += (X.str()).c_str();
			title += " ";
			title += (Y.str()).c_str();
			title += " ";
			title += (Z.str()).c_str();
			title += "/";
			title += FString::FromInt(LakeIndex);
			title += ".txt";

			UE_LOG(LogStats, Log, TEXT("%s"), *title);
			TArray <FString> LinesOfText;
			FFileHelper::LoadANSITextFileToStrings(*title, NULL, LinesOfText);
			LakeSliceToRead.Index = FCString::Atoi(*LinesOfText[0]);
			LakeSliceToRead.Volume = FCString::Atoi(*LinesOfText[1]);
			LakeSliceToRead.Coordinates.X = FCString::Atoi(*LinesOfText[2]);
			LakeSliceToRead.Coordinates.Y = FCString::Atoi(*LinesOfText[3]);
			LakeSliceToRead.Coordinates.Z = FCString::Atoi(*LinesOfText[4]);

			bool LakeBricks = true;
			bool BricksOnTheRegionFrontierAtX = false;
			bool BricksOnTheRegionFrontierAtMinusX = false;
			bool BricksOnTheRegionFrontierAtY = false;
			bool BricksOnTheRegionFrontierAtMinusY = false;

			int iterator = -1;
			while (iterator < LinesOfText.Num() - 6)
			{
				iterator++;
				FString CurrentLine = LinesOfText[iterator + 5];

				if (LakeBricks)
				{
					if (CurrentLine != "BricksOnTheRegionFrontierAtX")
						LakeSliceToRead.LakeBricks.Add(FCString::Atoi(*CurrentLine));
					else
					{
						LakeBricks = false;
						BricksOnTheRegionFrontierAtX = true;
					}
				}

				else if (BricksOnTheRegionFrontierAtX)
				{
					if (CurrentLine != "BricksOnTheRegionFrontierAtMinusX")
						LakeSliceToRead.BricksOnTheRegionFrontierAtX.Add(FCString::Atoi(*CurrentLine));
					else
					{
						BricksOnTheRegionFrontierAtX = false;
						BricksOnTheRegionFrontierAtMinusX = true;
					}
				}

				if (BricksOnTheRegionFrontierAtMinusX)
				{
					if (CurrentLine != "BricksOnTheRegionFrontierAtY")
						LakeSliceToRead.BricksOnTheRegionFrontierAtMinusX.Add(FCString::Atoi(*CurrentLine));
					else
					{
						BricksOnTheRegionFrontierAtMinusX = false;
						BricksOnTheRegionFrontierAtY = true;
					}
				}

				if (BricksOnTheRegionFrontierAtY)
				{
					if (CurrentLine != "BricksOnTheRegionFrontierAtMinusY")
						LakeSliceToRead.BricksOnTheRegionFrontierAtY.Add(FCString::Atoi(*CurrentLine));
					else
					{
						BricksOnTheRegionFrontierAtY = false;
						BricksOnTheRegionFrontierAtMinusY = true;
					}
				}

				if (BricksOnTheRegionFrontierAtMinusY)
				{
					LakeSliceToRead.BricksOnTheRegionFrontierAtMinusY.Add(FCString::Atoi(*CurrentLine));
				}
			}
		}
	}
	return true;
}
bool UBrickGridComponent::ReadRegion(FBrickRegion& RegionToRead)
{

	std::ostringstream X, Y, Z;
	X << RegionToRead.Coordinates.X;
	Y << RegionToRead.Coordinates.Y;
	Z << RegionToRead.Coordinates.Z;
	FString title, title2;
	title = "C:/Users/Migue/Desktop/Regions/";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();
	title2 = title;
	title2 += "Z";
	title2 += ".bin";
	title += ".bin";

	UE_LOG(LogStats, Log, TEXT("%s"), *title);
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*title))
	{
		return false;
	}
	else
	{/*
	 TArray <uint8> a;
	 FFileHelper::LoadFileToArray(a, *title2);
	 for (int iterator = 0; iterator<a.Num(); iterator++)
	 {
	 RegionToRead.MaxNonEmptyBrickRegionZs.Add(a[iterator]);//signed int8, for a total of 2048*2 bytes since it can be negative
	 }


	 TArray <uint8> CompressedBrickContent;
	 FFileHelper::LoadFileToArray(CompressedBrickContent, *title);

	 TArray<uint8>UncompressedBinaryArray;

	 UncompressedBinaryArray.SetNum((1 << (Parameters.BricksPerRegionLog2.X + Parameters.BricksPerRegionLog2.Y)) * 64 * 2);
	 Decompressuint8Array(CompressedBrickContent, UncompressedBinaryArray);
	 RegionToRead.BrickContents = UncompressedBinaryArray;*/

	}
	return true;
}

void UBrickGridComponent::Decompressuint8Array(TArray<uint8> &CompressedBinaryArray, TArray<uint8> &UncompressedBinaryArray)
{
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;


	strm.avail_in = CompressedBinaryArray.Num();
	strm.next_in = (Bytef *)CompressedBinaryArray.GetData();
	strm.avail_out = UncompressedBinaryArray.Num();
	strm.next_out = (Bytef *)UncompressedBinaryArray.GetData();

	// the actual DE-compression work.
	inflateInit(&strm);
	inflate(&strm, Z_FINISH);
	inflateEnd(&strm);
}
void UBrickGridComponent::Compressuint8Array(TArray<uint8> &CompressedBinaryArray, TArray<uint8> &UncompressedBinaryArray)
{
	CompressedBinaryArray.SetNum(UncompressedBinaryArray.Num() * 1023, true);

	//int ret;
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	strm.avail_in = UncompressedBinaryArray.Num();
	strm.next_in = (Bytef *)UncompressedBinaryArray.GetData();
	strm.avail_out = CompressedBinaryArray.Num();
	strm.next_out = (Bytef *)CompressedBinaryArray.GetData();


	// the actual compression work.
	deflateInit(&strm, Z_BEST_COMPRESSION);
	deflate(&strm, Z_FINISH);
	deflateEnd(&strm);

	// Shrink the array to minimum size
	CompressedBinaryArray.RemoveAt(strm.total_out, CompressedBinaryArray.Num() - strm.total_out, true);

}

void UBrickGridComponent::SaveRegion(FBrickRegion& RegionToSave)
{
	std::ostringstream X, Y, Z;
	X << RegionToSave.Coordinates.X;
	Y << RegionToSave.Coordinates.Y;
	Z << RegionToSave.Coordinates.Z;
	FString title, title2;
	title = "C:/Users/Migue/Desktop/Regions/";
	title += " ";
	title += (X.str()).c_str();
	title += " ";
	title += (Y.str()).c_str();
	title += " ";
	title += (Z.str()).c_str();

	title2 = title;
	title2 += "Z";
	title2 += ".bin";
	title += ".bin";

	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*title))
	{

		TArray<uint8> CompressedBinaryArray;
		Compressuint8Array(CompressedBinaryArray, RegionToSave.BrickContents);
		FFileHelper::SaveArrayToFile(CompressedBinaryArray, *title);

		TArray <uint8> a;
		for (int iterator = 0; iterator < RegionToSave.MaxNonEmptyBrickRegionZs.Num(); iterator++)
		{
			a.Add(RegionToSave.MaxNonEmptyBrickRegionZs[iterator]);//signed int8, for a total of 2048*2 bytes since it can be negative

		}
		FFileHelper::SaveArrayToFile(a, *title2);


	}
}
FBrickGridData UBrickGridComponent::GetData() const
{
	FBrickGridData Result;
	Result.Regions = Regions;
	return Result;
}

void UBrickGridComponent::SetData(const FBrickGridData& Data)
{
	Init(Parameters);
	Regions = Data.Regions;

	for (auto RegionIt = Regions.CreateIterator(); RegionIt; ++RegionIt)
	{
		// Compute the max non-empty brick map for the new regions.
		UpdateMaxNonEmptyBrickMap(*RegionIt, FInt3::Scalar(0), BricksPerRegion - FInt3::Scalar(1));

		// Recreate the region coordinate to index map.
		RegionCoordinatesToIndex.Add(RegionIt->Coordinates, RegionIt.GetIndex());
	}
}

FBrick UBrickGridComponent::GetBrick(const FInt3& BrickCoordinates) const
{
	if (FInt3::All(BrickCoordinates >= MinBrickCoordinates) && FInt3::All(BrickCoordinates <= MaxBrickCoordinates))
	{
		const FInt3 RegionCoordinates = BrickToRegionCoordinates(BrickCoordinates);
		const int32* const RegionIndex = RegionCoordinatesToIndex.Find(RegionCoordinates);
		if (RegionIndex != NULL)
		{
			const uint32 BrickIndex = BrickCoordinatesToRegionBrickIndex(RegionCoordinates, BrickCoordinates);
			const FBrickRegion& Region = Regions[*RegionIndex];
			return FBrick(Region.BrickContents[BrickIndex]);
		}
	}
	return FBrick(Parameters.EmptyMaterialIndex);
}

void UBrickGridComponent::GetBrickMaterialArray(const FInt3& MinBrickCoordinates, const FInt3& MaxBrickCoordinates, TArray<uint8>& OutBrickMaterials) const
{
	const FInt3 OutputSize = MaxBrickCoordinates - MinBrickCoordinates + FInt3::Scalar(1);
	const FInt3 MinRegionCoordinates = BrickToRegionCoordinates(MinBrickCoordinates);
	const FInt3 MaxRegionCoordinates = BrickToRegionCoordinates(MaxBrickCoordinates);
	for (int32 RegionY = MinRegionCoordinates.Y; RegionY <= MaxRegionCoordinates.Y; ++RegionY)
	{
		for (int32 RegionX = MinRegionCoordinates.X; RegionX <= MaxRegionCoordinates.X; ++RegionX)
		{
			for (int32 RegionZ = MinRegionCoordinates.Z; RegionZ <= MaxRegionCoordinates.Z; ++RegionZ)
			{
				const FInt3 RegionCoordinates(RegionX, RegionY, RegionZ);
				const int32* const RegionIndex = RegionCoordinatesToIndex.Find(RegionCoordinates);
				const FInt3 MinRegionBrickCoordinates = FInt3(RegionX, RegionY, RegionZ) * BricksPerRegion;
				const FInt3 MinOutputRegionBrickCoordinates = FInt3::Max(FInt3::Scalar(0), MinBrickCoordinates - MinRegionBrickCoordinates);
				const FInt3 MaxOutputRegionBrickCoordinates = FInt3::Min(BricksPerRegion - FInt3::Scalar(1), MaxBrickCoordinates - MinRegionBrickCoordinates);
				for (int32 RegionBrickY = MinOutputRegionBrickCoordinates.Y; RegionBrickY <= MaxOutputRegionBrickCoordinates.Y; ++RegionBrickY)
				{
					for (int32 RegionBrickX = MinOutputRegionBrickCoordinates.X; RegionBrickX <= MaxOutputRegionBrickCoordinates.X; ++RegionBrickX)
					{
						const int32 OutputX = MinRegionBrickCoordinates.X + RegionBrickX - MinBrickCoordinates.X;
						const int32 OutputY = MinRegionBrickCoordinates.Y + RegionBrickY - MinBrickCoordinates.Y;
						const int32 OutputMinZ = MinRegionBrickCoordinates.Z + MinOutputRegionBrickCoordinates.Z - MinBrickCoordinates.Z;
						const int32 OutputSizeZ = MaxOutputRegionBrickCoordinates.Z - MinOutputRegionBrickCoordinates.Z + 1;
						const uint32 OutputBaseBrickIndex = (OutputY * OutputSize.X + OutputX) * OutputSize.Z + OutputMinZ;
						uint32 RegionBaseBrickIndex = (((RegionBrickY << Parameters.BricksPerRegionLog2.X) + RegionBrickX) << Parameters.BricksPerRegionLog2.Z) + MinOutputRegionBrickCoordinates.Z;
						if (RegionIndex)
						{

							size_t a = RegionBaseBrickIndex;
							//if (a < Regions[*RegionIndex].BrickContents.Num() && Regions[*RegionIndex].BrickContents.Num() != 0)
							if (RegionBaseBrickIndex >= 196608)
							{
								//UE_LOG(LogStats, Log, TEXT("%d WORKED"), RegionBaseBrickIndex);
							}
							else
							{
								if (Regions[*RegionIndex].BrickContents.Num() != 0)
									FMemory::Memcpy(&OutBrickMaterials[OutputBaseBrickIndex], &Regions[*RegionIndex].BrickContents[RegionBaseBrickIndex], OutputSizeZ * sizeof(uint8));
							}
						}
						else
						{
							FMemory::Memset(&OutBrickMaterials[OutputBaseBrickIndex], Parameters.EmptyMaterialIndex, OutputSizeZ * sizeof(uint8));
						}
					}
				}
			}
		}
	}
}

void UBrickGridComponent::SetBrickMaterialArray(const FInt3& MinBrickCoordinates, const FInt3& MaxBrickCoordinates, const TArray<uint8>& BrickMaterials)
{
	const FInt3 InputSize = MaxBrickCoordinates - MinBrickCoordinates + FInt3::Scalar(1);
	const FInt3 MinRegionCoordinates = BrickToRegionCoordinates(MinBrickCoordinates);
	const FInt3 MaxRegionCoordinates = BrickToRegionCoordinates(MaxBrickCoordinates);
	for (int32 RegionY = MinRegionCoordinates.Y; RegionY <= MaxRegionCoordinates.Y; ++RegionY)
	{
		for (int32 RegionX = MinRegionCoordinates.X; RegionX <= MaxRegionCoordinates.X; ++RegionX)
		{
			for (int32 RegionZ = MinRegionCoordinates.Z; RegionZ <= MaxRegionCoordinates.Z; ++RegionZ)
			{
				const FInt3 RegionCoordinates(RegionX, RegionY, RegionZ);
				const int32* const RegionIndex = RegionCoordinatesToIndex.Find(RegionCoordinates);
				const FInt3 MinRegionBrickCoordinates = FInt3(RegionX, RegionY, RegionZ) * BricksPerRegion;
				const FInt3 MinInputRegionBrickCoordinates = FInt3::Max(FInt3::Scalar(0), MinBrickCoordinates - MinRegionBrickCoordinates);
				const FInt3 MaxInputRegionBrickCoordinates = FInt3::Min(BricksPerRegion - FInt3::Scalar(1), MaxBrickCoordinates - MinRegionBrickCoordinates);
				for (int32 RegionBrickY = MinInputRegionBrickCoordinates.Y; RegionBrickY <= MaxInputRegionBrickCoordinates.Y; ++RegionBrickY)
				{
					for (int32 RegionBrickX = MinInputRegionBrickCoordinates.X; RegionBrickX <= MaxInputRegionBrickCoordinates.X; ++RegionBrickX)
					{
						const int32 InputX = MinRegionBrickCoordinates.X + RegionBrickX - MinBrickCoordinates.X;
						const int32 InputY = MinRegionBrickCoordinates.Y + RegionBrickY - MinBrickCoordinates.Y;
						const int32 InputMinZ = MinRegionBrickCoordinates.Z + MinInputRegionBrickCoordinates.Z - MinBrickCoordinates.Z;
						const int32 InputSizeZ = MaxInputRegionBrickCoordinates.Z - MinInputRegionBrickCoordinates.Z + 1;
						const uint32 InputBaseBrickIndex = (InputY * InputSize.X + InputX) * InputSize.Z + InputMinZ;
						const uint32 RegionBaseBrickIndex = (((RegionBrickY << Parameters.BricksPerRegionLog2.X) + RegionBrickX) << Parameters.BricksPerRegionLog2.Z) + MinInputRegionBrickCoordinates.Z;
						if (RegionIndex)
						{
							FMemory::Memcpy(&Regions[*RegionIndex].BrickContents[RegionBaseBrickIndex], &BrickMaterials[InputBaseBrickIndex], InputSizeZ * sizeof(uint8));
						}
					}
				}
			}
		}
	}

	InvalidateChunkComponents(MinBrickCoordinates, MaxBrickCoordinates);
}

bool UBrickGridComponent::SetBrick(const FInt3& BrickCoordinates, int32 MaterialIndex)
{
	if (FInt3::All(BrickCoordinates >= MinBrickCoordinates) && FInt3::All(BrickCoordinates <= MaxBrickCoordinates) && MaterialIndex < Parameters.Materials.Num())
	{
		const FInt3 RegionCoordinates = BrickToRegionCoordinates(BrickCoordinates);
		const int32* const RegionIndex = RegionCoordinatesToIndex.Find(RegionCoordinates);
		if (RegionIndex != NULL)
		{
			const uint32 BrickIndex = BrickCoordinatesToRegionBrickIndex(RegionCoordinates, BrickCoordinates);
			FBrickRegion& Region = Regions[*RegionIndex];
			if (MaterialIndex == 1)
			{
				CreateLakeB(Region, BrickIndex);
			}
			Region.BrickContents[BrickIndex] = MaterialIndex;
			InvalidateChunkComponents(BrickCoordinates, BrickCoordinates);
			return true;
		}
	}
	return false;
}
void UBrickGridComponent::CreateLakeB(FBrickRegion& RegionToRead, int32 LakeIndex)
{

	UE_LOG(LogStats, Log, TEXT("Flooding Region %d,%d,%d, LakeIndex %d"), RegionToRead.Coordinates.X, RegionToRead.Coordinates.Y, RegionToRead.Coordinates.Z, LakeIndex);
	FBrickRegion::LakeSlice LakeToFlood;
	if (FromBrickCoordinatesFindRegionLake(RegionToRead, LakeIndex, LakeToFlood))
	{
		UE_LOG(LogStats, Log, TEXT("Real LakeIndex was %d"), LakeIndex);

		RegionAndLakeIndexCombo ThisRegionAndLakeIndexCombo;
		ThisRegionAndLakeIndexCombo.RegionCoordinates = RegionToRead.Coordinates;
		ThisRegionAndLakeIndexCombo.LakeIndex = LakeIndex;
		if (!ListOfVisitedRegionAndLakeIndexCombos.Contains(ThisRegionAndLakeIndexCombo))
		{
			ListOfVisitedRegionAndLakeIndexCombos.Add(ThisRegionAndLakeIndexCombo);

			for (int32 iterator = 0; iterator < LakeToFlood.LakeBricks.Num(); iterator++)
			{
				RegionToRead.BrickContents[LakeToFlood.LakeBricks[iterator]] = 9;
			}

			/*FLOOD LAKES ACROSS THE REGION FRONTIERS*/
			TArray<int32>IndexesOfLakesAcrossTheRegionFrontier;
			TArray<FInt3>CoordinatesOfSaidLakes;
			FindAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, LakeToFlood.BricksOnTheRegionFrontierAtX, IndexesOfLakesAcrossTheRegionFrontier, CoordinatesOfSaidLakes);
			FloodAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, "X", CoordinatesOfSaidLakes);
			FindAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, LakeToFlood.BricksOnTheRegionFrontierAtY, IndexesOfLakesAcrossTheRegionFrontier, CoordinatesOfSaidLakes);
			FloodAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, "Y", CoordinatesOfSaidLakes);
			FindAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, LakeToFlood.BricksOnTheRegionFrontierAtMinusX, IndexesOfLakesAcrossTheRegionFrontier, CoordinatesOfSaidLakes);
			FloodAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, "MinusX", CoordinatesOfSaidLakes);
			FindAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, LakeToFlood.BricksOnTheRegionFrontierAtMinusY, IndexesOfLakesAcrossTheRegionFrontier, CoordinatesOfSaidLakes);
			FloodAllIndexesOfLakesAcrossTheRegionFrontier(RegionToRead, "MinusY", CoordinatesOfSaidLakes);


			const FInt3 MinRegionBrickCoordinates = RegionToRead.Coordinates * BricksPerRegion;
			const FInt3 MaxRegionBrickCoordinates = MinRegionBrickCoordinates + BricksPerRegion - FInt3::Scalar(1);
			InvalidateChunkComponents(MinBrickCoordinates, MaxBrickCoordinates);
		}
		else
			UE_LOG(LogStats, Log, TEXT("BUT WAS ALREADY VISITED"));
	}
}
void UBrickGridComponent::FindAllIndexesOfLakesAcrossTheRegionFrontier(FBrickRegion& RegionToRead, TArray<int32>&LakeFrontier, TArray<int32>&ListOfLakeIndexesReadyToBeFlooded, TArray<FInt3> &BrickCoordinatesArray)
{
	ListOfLakeIndexesReadyToBeFlooded.Empty();
	BrickCoordinatesArray.Empty();
	for (int iterator = 0; iterator < LakeFrontier.Num(); iterator++)
	{
		int32 Index = RegionToRead.DuplicateArrayWhereLakeIndexesAreLinkedToVoxelData[LakeFrontier[iterator]];
		if (!ListOfLakeIndexesReadyToBeFlooded.Contains(Index))
		{
			ListOfLakeIndexesReadyToBeFlooded.Add(Index);
		}
	}
	for (int32 LocalZ = 0; LocalZ < BricksPerRegion.Z; ++LocalZ)
	{
		for (int32 LocalX = 0; LocalX < BricksPerRegion.X; ++LocalX)
		{
			for (int32 LocalY = 0; LocalY < BricksPerRegion.Y; ++LocalY)
			{
				int32 Index = ((LocalY * BricksPerRegion.X) + LocalX) * BricksPerRegion.Z + LocalZ;
				if (ListOfLakeIndexesReadyToBeFlooded.Contains(Index))
				{
					FInt3 LakeCoordinates(LocalX, LocalY, LocalZ);
					BrickCoordinatesArray.Add(LakeCoordinates);
				}
			}
		}
	}
}
void UBrickGridComponent::FloodAllIndexesOfLakesAcrossTheRegionFrontier(FBrickRegion& RegionToRead, FString FrontierSide, TArray<FInt3> &BrickCoordinates)
{

	FInt3 NeighboorRegionCoordinates;
	if (FrontierSide == "X")
	{
		FInt3 ExtraCoordinates(1, 0, 0);
		NeighboorRegionCoordinates = RegionToRead.Coordinates + ExtraCoordinates;
		const int32* const RegionIndex = RegionCoordinatesToIndex.Find(NeighboorRegionCoordinates);
		if (RegionIndex)
		{
			FBrickRegion& Region = Regions[*RegionIndex];
			for (int iterator = 0; iterator < BrickCoordinates.Num(); iterator++)
			{
				int32 Index = ((BrickCoordinates[iterator].Y * BricksPerRegion.X) + 0) * BricksPerRegion.Z + BrickCoordinates[iterator].Z;
				UE_LOG(LogStats, Log, TEXT("X... %d  %d  %d... %d"), NeighboorRegionCoordinates.X, NeighboorRegionCoordinates.Y, NeighboorRegionCoordinates.Z, Index);
				CreateLakeB(Region, Index);
			}
		}
	}
	if (FrontierSide == "MinusX")
	{
		FInt3 ExtraCoordinates(-1, 0, 0);
		NeighboorRegionCoordinates = RegionToRead.Coordinates + ExtraCoordinates;
		const int32* const RegionIndex = RegionCoordinatesToIndex.Find(NeighboorRegionCoordinates);
		if (RegionIndex)
		{
			FBrickRegion& Region = Regions[*RegionIndex]; for (int iterator = 0; iterator < BrickCoordinates.Num(); iterator++)
			{
				int32 Index = ((BrickCoordinates[iterator].Y * BricksPerRegion.X) + BrickCoordinates[iterator].X + 31) * BricksPerRegion.Z + BrickCoordinates[iterator].Z;

				UE_LOG(LogStats, Log, TEXT("MinusX... %d  %d  %d... %d"), NeighboorRegionCoordinates.X, NeighboorRegionCoordinates.Y, NeighboorRegionCoordinates.Z, Index);
				CreateLakeB(Region, Index);
			}
		}
	}
	if (FrontierSide == "Y")
	{
		FInt3 ExtraCoordinates(0, 1, 0);
		NeighboorRegionCoordinates = RegionToRead.Coordinates + ExtraCoordinates;

		const int32* const RegionIndex = RegionCoordinatesToIndex.Find(NeighboorRegionCoordinates);
		if (RegionIndex)
		{
			FBrickRegion& Region = Regions[*RegionIndex]; for (int iterator = 0; iterator < BrickCoordinates.Num(); iterator++)
			{
				int32 Index = ((0 * BricksPerRegion.X) + BrickCoordinates[iterator].X) * BricksPerRegion.Z + BrickCoordinates[iterator].Z;
				UE_LOG(LogStats, Log, TEXT("Y... %d  %d  %d... %d"), NeighboorRegionCoordinates.X, NeighboorRegionCoordinates.Y, NeighboorRegionCoordinates.Z, Index);
				CreateLakeB(Region, Index);
			}
		}
	}
	if (FrontierSide == "MinusY")
	{
		FInt3 ExtraCoordinates(0, -1, 0);
		NeighboorRegionCoordinates = RegionToRead.Coordinates + ExtraCoordinates;

		const int32* const RegionIndex = RegionCoordinatesToIndex.Find(NeighboorRegionCoordinates);
		if (RegionIndex)
		{
			FBrickRegion& Region = Regions[*RegionIndex]; for (int iterator = 0; iterator < BrickCoordinates.Num(); iterator++)
			{
				int32 Index = (((BrickCoordinates[iterator].Y + 31)* BricksPerRegion.X) + BrickCoordinates[iterator].X) * BricksPerRegion.Z + BrickCoordinates[iterator].Z;
				UE_LOG(LogStats, Log, TEXT("MinusY... %d  %d  %d... %d"), NeighboorRegionCoordinates.X, NeighboorRegionCoordinates.Y, NeighboorRegionCoordinates.Z, Index);
				CreateLakeB(Region, Index);
			}
		}
	}
	/*
	for (int iterator = 0; iterator < IndexesOfLakesAcrossTheRegionFrontier.Num(); iterator++)
	{
	CreateLakeB(RegionToRead, IndexesOfLakesAcrossTheRegionFrontier[iterator]);
	}
	*/
}

void UBrickGridComponent::UpdateMaxNonEmptyBrickMap(FBrickRegion& Region, const FInt3 MinDirtyRegionBrickCoordinates, const FInt3 MaxDirtyRegionBrickCoordinates) const
{

	// Allocate the map.
	if (!Region.MaxNonEmptyBrickRegionZs.Num())
	{
		Region.MaxNonEmptyBrickRegionZs.SetNumUninitialized(1 << (Parameters.BricksPerRegionLog2.X + Parameters.BricksPerRegionLog2.Y));
	}

	// For each XY in the chunk, find the highest non-empty brick between the bottom of the chunk and the top of the grid.
	for (int32 RegionBrickY = MinDirtyRegionBrickCoordinates.Y; RegionBrickY <= MaxDirtyRegionBrickCoordinates.Y; ++RegionBrickY)
	{
		for (int32 RegionBrickX = MinDirtyRegionBrickCoordinates.X; RegionBrickX <= MaxDirtyRegionBrickCoordinates.X; ++RegionBrickX)
		{
			int32 MaxNonEmptyRegionBrickZ = BricksPerRegion.Z - 1;
			for (; MaxNonEmptyRegionBrickZ >= 0; --MaxNonEmptyRegionBrickZ)
			{
				const uint32 RegionBrickIndex = (((RegionBrickY << Parameters.BricksPerRegionLog2.X) + RegionBrickX) << Parameters.BricksPerRegionLog2.Z) + MaxNonEmptyRegionBrickZ;
				size_t a = RegionBrickIndex;
				if (a < Region.BrickContents.Num())
				{
					if (Region.BrickContents[RegionBrickIndex] != Parameters.EmptyMaterialIndex)
					{
						break;
					}
				}
			}
			Region.MaxNonEmptyBrickRegionZs[(RegionBrickY << Parameters.BricksPerRegionLog2.X) + RegionBrickX] = (int8)MaxNonEmptyRegionBrickZ;

			Region.MaxNonEmptyBrickZofRegion = MaxNonEmptyRegionBrickZ;
			FInt3 NewCoordinates(RegionBrickX, RegionBrickY, MaxNonEmptyRegionBrickZ);
		}
	}
}






void UBrickGridComponent::GetMaxNonEmptyBrickZ(const FInt3& MinBrickCoordinates, const FInt3& MaxBrickCoordinates, TArray<int8>& OutHeightMap) const
{
	const FInt3 OutputSize = MaxBrickCoordinates - MinBrickCoordinates + FInt3::Scalar(1);
	const FInt3 MinRegionCoordinates = BrickToRegionCoordinates(MinBrickCoordinates);
	const FInt3 MaxRegionCoordinates = BrickToRegionCoordinates(MaxBrickCoordinates);
	const FInt3 TopMaxRegionCoordinates = FInt3(MaxRegionCoordinates.X, MaxRegionCoordinates.Y, Parameters.MaxRegionCoordinates.Z);
	for (int32 RegionY = MinRegionCoordinates.Y; RegionY <= TopMaxRegionCoordinates.Y; ++RegionY)
	{
		for (int32 RegionX = MinRegionCoordinates.X; RegionX <= TopMaxRegionCoordinates.X; ++RegionX)
		{
			TArray<const FBrickRegion*> ZRegions;
			ZRegions.Empty(TopMaxRegionCoordinates.Z - MinRegionCoordinates.Z + 1);
			for (int32 RegionZ = MinRegionCoordinates.Z; RegionZ <= TopMaxRegionCoordinates.Z; ++RegionZ)
			{
				const FInt3 RegionCoordinates(RegionX, RegionY, RegionZ);
				const int32* const RegionIndex = RegionCoordinatesToIndex.Find(RegionCoordinates);
				if (RegionIndex)
				{
					ZRegions.Add(&Regions[*RegionIndex]);
				}
			}
			const FInt3 MinRegionBrickCoordinates = FInt3(RegionX, RegionY, MinRegionCoordinates.Z) * BricksPerRegion;
			const FInt3 MinOutputRegionBrickCoordinates = FInt3::Max(FInt3::Scalar(0), MinBrickCoordinates - MinRegionBrickCoordinates);
			const FInt3 MaxOutputRegionBrickCoordinates = FInt3::Min(BricksPerRegion - FInt3::Scalar(1), MaxBrickCoordinates - MinRegionBrickCoordinates);
			for (int32 RegionBrickY = MinOutputRegionBrickCoordinates.Y; RegionBrickY <= MaxOutputRegionBrickCoordinates.Y; ++RegionBrickY)
			{
				for (int32 RegionBrickX = MinOutputRegionBrickCoordinates.X; RegionBrickX <= MaxOutputRegionBrickCoordinates.X; ++RegionBrickX)
				{
					int32 MaxNonEmptyBrickZ = UBrickGridComponent::MinBrickCoordinates.Z - 1;
					for (int32 RegionZIndex = ZRegions.Num() - 1; RegionZIndex >= 0; --RegionZIndex)
					{
						const FBrickRegion& Region = *ZRegions[RegionZIndex];
						const int8 RegionMaxNonEmptyZ = Region.MaxNonEmptyBrickRegionZs[(RegionBrickY << Parameters.BricksPerRegionLog2.X) + RegionBrickX];
						if (RegionMaxNonEmptyZ != -1)
						{
							MaxNonEmptyBrickZ = Region.Coordinates.Z * BricksPerRegion.Z + (int32)RegionMaxNonEmptyZ;
							break;
						}
					}

					const int32 OutputX = MinRegionBrickCoordinates.X + RegionBrickX - MinBrickCoordinates.X;
					const int32 OutputY = MinRegionBrickCoordinates.Y + RegionBrickY - MinBrickCoordinates.Y;
					OutHeightMap[OutputY * OutputSize.X + OutputX] = (int8)FMath::Clamp(MaxNonEmptyBrickZ - MinBrickCoordinates.Z, -1, 127);
				}
			}
		}
	}
}

void UBrickGridComponent::InvalidateChunkComponents(const FInt3& MinBrickCoordinates, const FInt3& MaxBrickCoordinates)
{
	// Expand the brick box by 1 brick so that bricks facing the one being invalidated are also updated.
	const FInt3 FacingExpansionExtent = FInt3::Scalar(1);

	// Update the region non-empty brick max Z maps.
	const FInt3 MinRegionCoordinates = BrickToRegionCoordinates(MinBrickCoordinates);
	const FInt3 MaxRegionCoordinates = BrickToRegionCoordinates(MaxBrickCoordinates);
	for (int32 RegionZ = Parameters.MinRegionCoordinates.Z; RegionZ <= MaxRegionCoordinates.Z; ++RegionZ)
	{
		for (int32 RegionY = MinRegionCoordinates.Y; RegionY <= MaxRegionCoordinates.Y; ++RegionY)
		{
			for (int32 RegionX = MinRegionCoordinates.X; RegionX <= MaxRegionCoordinates.X; ++RegionX)
			{
				const FInt3 RegionCoordinates(RegionX, RegionY, RegionZ);
				const int32* const RegionIndex = RegionCoordinatesToIndex.Find(RegionCoordinates);
				if (RegionIndex)
				{
					const FInt3 MinRegionBrickCoordinates = RegionCoordinates * BricksPerRegion;
					const FInt3 MinDirtyRegionBrickCoordinates = FInt3::Max(FInt3::Scalar(0), MinBrickCoordinates - MinRegionBrickCoordinates);
					const FInt3 MaxDirtyRegionBrickCoordinates = FInt3::Min(BricksPerRegion - FInt3::Scalar(1), MaxBrickCoordinates - MinRegionBrickCoordinates);
					UpdateMaxNonEmptyBrickMap(Regions[*RegionIndex], MinDirtyRegionBrickCoordinates, MaxDirtyRegionBrickCoordinates);
				}
			}
		}
	}

	// Invalidate render components. Note that because of ambient occlusion, the render chunks need to be invalidated all the way to the bottom of the grid!
	const FInt3 AmbientOcclusionExpansionExtent = FInt3::Scalar(Parameters.AmbientOcclusionBlurRadius);
	const FInt3 RenderExpansionExtent = AmbientOcclusionExpansionExtent + FacingExpansionExtent;
	const FInt3 MinRenderChunkCoordinates = BrickToRenderChunkCoordinates(MinBrickCoordinates - RenderExpansionExtent);
	const FInt3 MaxRenderChunkCoordinates = BrickToRenderChunkCoordinates(MaxBrickCoordinates + RenderExpansionExtent);
	for (int32 ChunkX = MinRenderChunkCoordinates.X; ChunkX <= MaxRenderChunkCoordinates.X; ++ChunkX)
	{
		for (int32 ChunkY = MinRenderChunkCoordinates.Y; ChunkY <= MaxRenderChunkCoordinates.Y; ++ChunkY)
		{
			for (int32 ChunkZ = UBrickGridComponent::MinBrickCoordinates.Z; ChunkZ <= MaxRenderChunkCoordinates.Z; ++ChunkZ)
			{
				UBrickRenderComponent* RenderComponent = RenderChunkCoordinatesToComponent.FindRef(FInt3(ChunkX, ChunkY, ChunkZ));
				if (RenderComponent)
				{
					if (ChunkZ >= MinRenderChunkCoordinates.Z)
					{
						RenderComponent->MarkRenderStateDirty();
					}
					else
					{
						// If the chunk only needs to be invalidate to update its ambient occlusion, defer it as a low priority update.
						RenderComponent->HasLowPriorityUpdatePending = true;
					}
				}
			}
		}
	}

	// Invalidate collision components.
	const FInt3 MinCollisionChunkCoordinates = BrickToCollisionChunkCoordinates(MinBrickCoordinates - FacingExpansionExtent);
	const FInt3 MaxCollisionChunkCoordinates = BrickToCollisionChunkCoordinates(MaxBrickCoordinates + FacingExpansionExtent);
	for (int32 ChunkX = MinCollisionChunkCoordinates.X; ChunkX <= MaxCollisionChunkCoordinates.X; ++ChunkX)
	{
		for (int32 ChunkY = MinCollisionChunkCoordinates.Y; ChunkY <= MaxCollisionChunkCoordinates.Y; ++ChunkY)
		{
			for (int32 ChunkZ = MinCollisionChunkCoordinates.Z; ChunkZ <= MaxCollisionChunkCoordinates.Z; ++ChunkZ)
			{
				UBrickCollisionComponent* CollisionComponent = CollisionChunkCoordinatesToComponent.FindRef(FInt3(ChunkX, ChunkY, ChunkZ));
				if (CollisionComponent)
				{
					CollisionComponent->MarkRenderStateDirty();
				}
			}
		}
	}
}

void UBrickGridComponent::Update(const FVector& WorldViewPosition, float MaxDrawDistance, float MaxCollisionDistance, float MaxDesiredUpdateTime, FBrickGrid_InitRegion OnInitRegion)
{

	const FVector LocalViewPosition = GetComponentTransform().InverseTransformPosition(WorldViewPosition);
	const float LocalMaxDrawDistance = FMath::Max(0.0f, MaxDrawDistance / GetComponentTransform().GetScale3D().GetMin());
	const float LocalMaxCollisionDistance = FMath::Max(0.0f, MaxCollisionDistance / GetComponentTransform().GetScale3D().GetMin());
	const float LocalMaxDrawAndCollisionDistance = FMath::Max(LocalMaxDrawDistance, LocalMaxCollisionDistance);

	const double StartTime = FPlatformTime::Seconds();
	double DeltaBigAss = 0;

	// Initialize any regions that are closer to the viewer than the draw or collision distance.
	// Include an additional ring of regions around what is being drawn or colliding so it has plenty of frames to spread initialization over before the data is needed.r
	const FInt3 MinInitRegionCoordinates = FInt3::Max(Parameters.MinRegionCoordinates, BrickToRegionCoordinates(FInt3::Floor(LocalViewPosition - FVector(LocalMaxDrawAndCollisionDistance))) - FInt3::Scalar(1));
	const FInt3 MaxInitRegionCoordinates = FInt3::Min(Parameters.MaxRegionCoordinates, BrickToRegionCoordinates(FInt3::Ceil(LocalViewPosition + FVector(LocalMaxDrawAndCollisionDistance))) + FInt3::Scalar(1));
	const float RegionExpansionRadius = BricksPerRegion.ToFloat().GetMin();
	for (int32 RegionZ = MinInitRegionCoordinates.Z; RegionZ <= MaxInitRegionCoordinates.Z && (FPlatformTime::Seconds() - StartTime - DeltaBigAss) < MaxDesiredUpdateTime; ++RegionZ)
	{
		for (int32 RegionY = MinInitRegionCoordinates.Y; RegionY <= MaxInitRegionCoordinates.Y && (FPlatformTime::Seconds() - StartTime - DeltaBigAss) < MaxDesiredUpdateTime; ++RegionY)
		{
			for (int32 RegionX = MinInitRegionCoordinates.X; RegionX <= MaxInitRegionCoordinates.X && (FPlatformTime::Seconds() - StartTime - DeltaBigAss) < MaxDesiredUpdateTime; ++RegionX)
			{
				const FInt3 RegionCoordinates(RegionX, RegionY, RegionZ);
				const FBox RegionBounds((RegionCoordinates * BricksPerRegion).ToFloat(), ((RegionCoordinates + FInt3::Scalar(1)) * BricksPerRegion).ToFloat());
				if (RegionBounds.ComputeSquaredDistanceToPoint(LocalViewPosition) < FMath::Square(LocalMaxDrawAndCollisionDistance + RegionExpansionRadius))
				{
					const int32* const RegionIndex = RegionCoordinatesToIndex.Find(RegionCoordinates);
					if (!RegionIndex)//if it has not been loaded on RAM memory
					{

						const int32 RegionIndex = Regions.Num();
						FBrickRegion& Region = *new(Regions)FBrickRegion;
						Region.Coordinates = RegionCoordinates;
						std::ostringstream X, Y, Z;
						X << Region.Coordinates.X;
						Y << Region.Coordinates.Y;
						Z << Region.Coordinates.Z;
						FString title;
						title = "C:/Users/Migue/Desktop/Regions/";
						title += (X.str()).c_str();
						title += " ";
						title += (Y.str()).c_str();
						title += " ";
						title += (Z.str()).c_str();
						title += ".bin";
						if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*title))
						{
							if (Region.Coordinates.Y != 0)
							{
								UE_LOG(LogStats, Log, TEXT("%d  %d  %d WORKED"), Region.Coordinates.X, Region.Coordinates.Y, Region.Coordinates.Z);
								RegionCoordinatesToIndex.Add(RegionCoordinates, RegionIndex);
								//OnInitRegion.Execute(RegionCoordinates);
								//ReadRegion(Region.Coordinates, Region);
								ReadRegionUncompressed(Region);
							}
						}/*
						 if (ReadRegion(Region.Coordinates, Region) == false)//if it has not been saved on disk yet.
						 {
						 // Initialize the region's bricks to the empty material.
						 Region.BrickContents.Init(Parameters.EmptyMaterialIndex, 1 << Parameters.BricksPerRegionLog2.SumComponents());

						 // Compute the region's non-empty height map.
						 UpdateMaxNonEmptyBrickMap(Region, FInt3::Scalar(0), BricksPerRegion - FInt3::Scalar(1));


						 // Add the region to the coordinate map.
						 RegionCoordinatesToIndex.Add(RegionCoordinates, RegionIndex);

						 // Call the InitRegion delegate for the new region.
						 OnInitRegion.Execute(RegionCoordinates);

						 SaveRegion(Region);

						 }
						 else//if the region had been saved onto disk previously
						 {

						 UE_LOG(LogStats, Log, TEXT("%s WORKED"), *title);
						 RegionCoordinatesToIndex.Add(RegionCoordinates, RegionIndex);
						 //OnInitRegion.Execute(RegionCoordinates);
						 //ReadRegion(Region.Coordinates, Region);
						 ReadRegionUncompressed(Region.Coordinates, Region);
						 }*/
						/*
						Region.BrickContents.Init(Parameters.EmptyMaterialIndex, 1 << Parameters.BricksPerRegionLog2.SumComponents());
						ReadRegionUncompressed(Region.Coordinates, Region);
						UpdateMaxNonEmptyBrickMap(Region, FInt3::Scalar(0), BricksPerRegion - FInt3::Scalar(1));
						RegionCoordinatesToIndex.Add(RegionCoordinates, RegionIndex);*/
					}
				}
			}
		}
	}

	// Create render components for any chunks closer to the viewer than the draw distance, and destroy any that are no longer inside the draw distance.
	// Do this visibility check in 2D so the chunks underneath those on the horizon are also drawn even if they are too far.
	const FInt3 MinRenderChunkCoordinates = BrickToRenderChunkCoordinates(FInt3::Max(MinBrickCoordinates, FInt3::Floor(LocalViewPosition - FVector(LocalMaxDrawDistance))));
	const FInt3 MaxRenderChunkCoordinates = BrickToRenderChunkCoordinates(FInt3::Min(MaxBrickCoordinates, FInt3::Ceil(LocalViewPosition + FVector(LocalMaxDrawDistance))));
	for (auto ChunkIt = RenderChunkCoordinatesToComponent.CreateIterator(); ChunkIt; ++ChunkIt)
	{
		const FInt3 MinChunkBrickCoordinates = ChunkIt.Key() * BricksPerRenderChunk;
		const FInt3 MaxChunkBrickCoordinates = MinChunkBrickCoordinates + BricksPerRenderChunk - FInt3::Scalar(1);
		const FBox ChunkBounds(
			FInt3(MinChunkBrickCoordinates.X, MinChunkBrickCoordinates.Y, MinBrickCoordinates.Z).ToFloat(),
			FInt3(MinChunkBrickCoordinates.X, MinChunkBrickCoordinates.Y, MaxBrickCoordinates.Z).ToFloat()
			);
		if (ChunkBounds.ComputeSquaredDistanceToPoint(LocalViewPosition) > FMath::Square(LocalMaxDrawDistance))
		{
			ChunkIt.Value()->DetachFromParent();
			ChunkIt.Value()->DestroyComponent();
			ChunkIt.RemoveCurrent();
		}
	}
	for (int32 ChunkZ = BrickToRenderChunkCoordinates(MinBrickCoordinates).Z; ChunkZ <= BrickToRenderChunkCoordinates(MaxBrickCoordinates).Z && (FPlatformTime::Seconds() - StartTime) < MaxDesiredUpdateTime; ++ChunkZ)
	{
		for (int32 ChunkY = MinRenderChunkCoordinates.Y; ChunkY <= MaxRenderChunkCoordinates.Y && (FPlatformTime::Seconds() - StartTime) < MaxDesiredUpdateTime; ++ChunkY)
		{
			for (int32 ChunkX = MinRenderChunkCoordinates.X; ChunkX <= MaxRenderChunkCoordinates.X && (FPlatformTime::Seconds() - StartTime) < MaxDesiredUpdateTime; ++ChunkX)
			{
				const FInt3 ChunkCoordinates(ChunkX, ChunkY, ChunkZ);
				const FInt3 MinChunkBrickCoordinates = ChunkCoordinates * BricksPerRenderChunk;
				const FInt3 MaxChunkBrickCoordinates = MinChunkBrickCoordinates + BricksPerRenderChunk - FInt3::Scalar(1);
				const FBox ChunkBounds(
					FInt3(MinChunkBrickCoordinates.X, MinChunkBrickCoordinates.Y, MinBrickCoordinates.Z).ToFloat(),
					FInt3(MinChunkBrickCoordinates.X, MinChunkBrickCoordinates.Y, MaxBrickCoordinates.Z).ToFloat()
					);
				if (ChunkBounds.ComputeSquaredDistanceToPoint(LocalViewPosition) < FMath::Square(LocalMaxDrawDistance))
				{
					UBrickRenderComponent* RenderComponent = RenderChunkCoordinatesToComponent.FindRef(ChunkCoordinates);
					if (!RenderComponent)
					{
						// Initialize a new chunk component.
						RenderComponent = NewObject<UBrickRenderComponent>(GetOwner());
						RenderComponent->Grid = this;
						RenderComponent->Coordinates = ChunkCoordinates;

						// Set the component transform and register it.
						RenderComponent->SetRelativeLocation((ChunkCoordinates * BricksPerRenderChunk).ToFloat());
						RenderComponent->AttachTo(this);
						RenderComponent->RegisterComponent();

						// Add the chunk to the coordinate map and visible chunk array.
						RenderChunkCoordinatesToComponent.Add(ChunkCoordinates, RenderComponent);
					}

					// Flush low-priority pending updates to render components.
					if (RenderComponent->HasLowPriorityUpdatePending)
					{
						RenderComponent->MarkRenderStateDirty();
						RenderComponent->HasLowPriorityUpdatePending = false;
					}
				}
			}
		}
	}

	// Create collision components for any chunks closer to the viewer than the collision distance, and destroy any that are no longer inside the draw distance.
	const FInt3 MinCollisionChunkCoordinates = BrickToCollisionChunkCoordinates(FInt3::Max(MinBrickCoordinates, FInt3::Floor(LocalViewPosition - FVector(LocalMaxCollisionDistance))));
	const FInt3 MaxCollisionChunkCoordinates = BrickToCollisionChunkCoordinates(FInt3::Min(MaxBrickCoordinates, FInt3::Ceil(LocalViewPosition + FVector(LocalMaxCollisionDistance))));
	const float LocalCollisionChunkRadius = BricksPerCollisionChunk.ToFloat().Size();
	for (auto ChunkIt = CollisionChunkCoordinatesToComponent.CreateIterator(); ChunkIt; ++ChunkIt)
	{
		const FBox ChunkBounds((ChunkIt.Key() * BricksPerCollisionChunk).ToFloat(), ((ChunkIt.Key() + FInt3::Scalar(1)) * BricksPerCollisionChunk).ToFloat());
		if (FInt3::Any(ChunkIt.Key() < MinCollisionChunkCoordinates)
			|| FInt3::Any(ChunkIt.Key() > MaxCollisionChunkCoordinates)
			|| ChunkBounds.ComputeSquaredDistanceToPoint(LocalViewPosition) > FMath::Square(LocalMaxCollisionDistance))
		{
			ChunkIt.Value()->DetachFromParent();
			ChunkIt.Value()->DestroyComponent();
			ChunkIt.RemoveCurrent();
		}
	}
	for (int32 ChunkZ = MinCollisionChunkCoordinates.Z; ChunkZ <= MaxCollisionChunkCoordinates.Z; ++ChunkZ)
	{
		for (int32 ChunkY = MinCollisionChunkCoordinates.Y; ChunkY <= MaxCollisionChunkCoordinates.Y; ++ChunkY)
		{
			for (int32 ChunkX = MinCollisionChunkCoordinates.X; ChunkX <= MaxCollisionChunkCoordinates.X; ++ChunkX)
			{
				const FInt3 ChunkCoordinates(ChunkX, ChunkY, ChunkZ);
				const FBox ChunkBounds((ChunkCoordinates * BricksPerCollisionChunk).ToFloat(), ((ChunkCoordinates + FInt3::Scalar(1)) * BricksPerCollisionChunk).ToFloat());
				if (ChunkBounds.ComputeSquaredDistanceToPoint(LocalViewPosition) < FMath::Square(LocalMaxCollisionDistance))
				{
					if (!CollisionChunkCoordinatesToComponent.FindRef(ChunkCoordinates))
					{
						// Initialize a new chunk component.
						UBrickCollisionComponent* Chunk = NewObject<UBrickCollisionComponent>(GetOwner());
						Chunk->Grid = this;
						Chunk->Coordinates = ChunkCoordinates;

						// Set the component transform and register it.
						Chunk->SetRelativeLocation((ChunkCoordinates * BricksPerCollisionChunk).ToFloat());
						Chunk->AttachTo(this);
						Chunk->RegisterComponent();

						// Add the chunk to the coordinate map and visible chunk array.
						CollisionChunkCoordinatesToComponent.Add(ChunkCoordinates, Chunk);
					}
				}
			}
		}
	}
}

FBoxSphereBounds UBrickGridComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	// Return a bounds that fills the world.
	return FBoxSphereBounds(FVector(0, 0, 0), FVector(1, 1, 1) * HALF_WORLD_MAX, FMath::Sqrt(3.0f * HALF_WORLD_MAX));
}

FBrickGridParameters::FBrickGridParameters()
	: EmptyMaterialIndex(0)
	, BricksPerRegionLog2(5, 5, 7)
	, RenderChunksPerRegionLog2(0, 0, 2)
	, CollisionChunksPerRegionLog2(1, 1, 2)
	, MinRegionCoordinates(-1024, -1024, 0)
	, MaxRegionCoordinates(+1024, +1024, 0)
	, AmbientOcclusionBlurRadius(2)
{
	Materials.Add(FBrickMaterial());
}

UBrickGridComponent::UBrickGridComponent(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = true;

	Init(Parameters);
}

void UBrickGridComponent::OnRegister()
{
	Super::OnRegister();

	for (auto ChunkIt = RenderChunkCoordinatesToComponent.CreateConstIterator(); ChunkIt; ++ChunkIt)
	{
		ChunkIt.Value()->RegisterComponent();
	}
	for (auto ChunkIt = CollisionChunkCoordinatesToComponent.CreateConstIterator(); ChunkIt; ++ChunkIt)
	{
		ChunkIt.Value()->RegisterComponent();
	}
}

void UBrickGridComponent::OnUnregister()
{
	for (auto ChunkIt = RenderChunkCoordinatesToComponent.CreateConstIterator(); ChunkIt; ++ChunkIt)
	{
		ChunkIt.Value()->UnregisterComponent();
	}
	for (auto ChunkIt = CollisionChunkCoordinatesToComponent.CreateConstIterator(); ChunkIt; ++ChunkIt)
	{
		ChunkIt.Value()->UnregisterComponent();
	}

	Super::OnUnregister();
}