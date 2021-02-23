// Fill out your copyright notice in the Description page of Project Settings.
#include "ShaderPatch/ShaderPatchProxy.h"
#include "FlibHotPatcherEditorHelper.h"
#include "ShaderPatch/FlibShaderPatchHelper.h"

#define LOCTEXT_NAMESPACE "HotPatcherShaderPatchProxy"

static FString ShaderExtension = TEXT(".ushaderbytecode");
static FString GetCodeArchiveFilename(const FString& BaseDir, const FString& LibraryName, FName Platform)
{
	return BaseDir / FString::Printf(TEXT("ShaderArchive-%s-"), *LibraryName) + Platform.ToString() + ShaderExtension;
}

bool UShaderPatchProxy::DoExport()
{
	bool bStatus = false;
	for(const auto& PlatformConfig:GetSettingObject()->ShaderPatchConfigs)
	{
		FString SaveToPath = FPaths::Combine(FPaths::ConvertRelativePathToFull(GetSettingObject()->SaveTo.Path),UFlibPatchParserHelper::GetEnumNameByValue(PlatformConfig.Platform));
		bool bCreateStatus = UFlibShaderPatchHelper::CreateShaderCodePatch(
        UFlibShaderPatchHelper::ConvDirectoryPathToStr(PlatformConfig.OldMetadataDir),
        FPaths::ConvertRelativePathToFull(PlatformConfig.NewMetadataDir.Path),
        SaveToPath,
        PlatformConfig.bNativeFormat
        );

		auto GetShaderPatchFormatLambda = [](const FString& ShaderPatchDir)->TMap<FName, TSet<FString>>
		{
			TMap<FName, TSet<FString>> FormatLibraryMap;
			TArray<FString> LibraryFiles;
			IFileManager::Get().FindFiles(LibraryFiles, *(ShaderPatchDir), *ShaderExtension);
	
			for (FString const& Path : LibraryFiles)
			{
				FString Name = FPaths::GetBaseFilename(Path);
				if (Name.RemoveFromStart(TEXT("ShaderArchive-")))
				{
					TArray<FString> Components;
					if (Name.ParseIntoArray(Components, TEXT("-")) == 2)
					{
						FName Format(*Components[1]);
						TSet<FString>& Libraries = FormatLibraryMap.FindOrAdd(Format);
						Libraries.Add(Components[0]);
					}
				}
			}
			return FormatLibraryMap;
		};
		
		if(bCreateStatus)
		{
			TMap<FName,TSet<FString>> ShaderFormatLibraryMap  = GetShaderPatchFormatLambda(SaveToPath);
			FText Msg = LOCTEXT("GeneratedShaderPatch", "Successd to Generated the Shader Patch.");
			TArray<FName> FormatNames;
			ShaderFormatLibraryMap.GetKeys(FormatNames);
			for(const auto& FormatName:FormatNames)
			{
				TArray<FString> LibraryNames= ShaderFormatLibraryMap[FormatName].Array();
				for(const auto& LibrartName:LibraryNames)
				{
					FString OutputFilePath = GetCodeArchiveFilename(SaveToPath, LibrartName, FormatName);
					if(FPaths::FileExists(OutputFilePath))
					{
						bStatus = true;
						if(!IsRunningCommandlet())
						{
							UFlibHotPatcherEditorHelper::CreateSaveFileNotify(Msg, OutputFilePath);
						}
					}
				}
			}
		}
	}
	return bStatus;
}

#undef LOCTEXT_NAMESPACE