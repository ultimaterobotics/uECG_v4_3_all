##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=ecg_monitor
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=the_3d6
Date                   :=31/07/19
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)NDEBUG 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="ecg_monitor.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  $(shell pkg-config --libs gtk+-2.0)
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -O2 -Wall $(Preprocessors)
CFLAGS   :=  -O2 -Wall $(shell pkg-config --cflags gtk+-2.0) $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
Objects0=$(IntermediateDirectory)/main.c$(ObjectSuffix) $(IntermediateDirectory)/ui_functions.c$(ObjectSuffix) $(IntermediateDirectory)/sys_functions.c$(ObjectSuffix) $(IntermediateDirectory)/serial_functions.c$(ObjectSuffix) $(IntermediateDirectory)/ui_base.c$(ObjectSuffix) $(IntermediateDirectory)/drawing.c$(ObjectSuffix) $(IntermediateDirectory)/simplechart.c$(ObjectSuffix) $(IntermediateDirectory)/packet_parser.c$(ObjectSuffix) $(IntermediateDirectory)/device_functions.c$(ObjectSuffix) $(IntermediateDirectory)/fft.c$(ObjectSuffix) $(IntermediateDirectory)/sft.c$(ObjectSuffix) \
	$(IntermediateDirectory)/spectrogram.c$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Release || $(MakeDirCommand) ./Release


$(IntermediateDirectory)/.d:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main.c$(ObjectSuffix): main.c $(IntermediateDirectory)/main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.c$(DependSuffix): main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/main.c$(DependSuffix) -MM main.c

$(IntermediateDirectory)/main.c$(PreprocessSuffix): main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.c$(PreprocessSuffix) main.c

$(IntermediateDirectory)/ui_functions.c$(ObjectSuffix): ui_functions.c $(IntermediateDirectory)/ui_functions.c$(DependSuffix)
	$(CC) $(SourceSwitch) "ui_functions.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ui_functions.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ui_functions.c$(DependSuffix): ui_functions.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ui_functions.c$(ObjectSuffix) -MF$(IntermediateDirectory)/ui_functions.c$(DependSuffix) -MM ui_functions.c

$(IntermediateDirectory)/ui_functions.c$(PreprocessSuffix): ui_functions.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ui_functions.c$(PreprocessSuffix) ui_functions.c

$(IntermediateDirectory)/sys_functions.c$(ObjectSuffix): sys_functions.c $(IntermediateDirectory)/sys_functions.c$(DependSuffix)
	$(CC) $(SourceSwitch) "sys_functions.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sys_functions.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sys_functions.c$(DependSuffix): sys_functions.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sys_functions.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sys_functions.c$(DependSuffix) -MM sys_functions.c

$(IntermediateDirectory)/sys_functions.c$(PreprocessSuffix): sys_functions.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sys_functions.c$(PreprocessSuffix) sys_functions.c

$(IntermediateDirectory)/serial_functions.c$(ObjectSuffix): serial_functions.c $(IntermediateDirectory)/serial_functions.c$(DependSuffix)
	$(CC) $(SourceSwitch) "serial_functions.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/serial_functions.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/serial_functions.c$(DependSuffix): serial_functions.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/serial_functions.c$(ObjectSuffix) -MF$(IntermediateDirectory)/serial_functions.c$(DependSuffix) -MM serial_functions.c

$(IntermediateDirectory)/serial_functions.c$(PreprocessSuffix): serial_functions.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/serial_functions.c$(PreprocessSuffix) serial_functions.c

$(IntermediateDirectory)/ui_base.c$(ObjectSuffix): ui_base.c $(IntermediateDirectory)/ui_base.c$(DependSuffix)
	$(CC) $(SourceSwitch) "ui_base.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/ui_base.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/ui_base.c$(DependSuffix): ui_base.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/ui_base.c$(ObjectSuffix) -MF$(IntermediateDirectory)/ui_base.c$(DependSuffix) -MM ui_base.c

$(IntermediateDirectory)/ui_base.c$(PreprocessSuffix): ui_base.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/ui_base.c$(PreprocessSuffix) ui_base.c

$(IntermediateDirectory)/drawing.c$(ObjectSuffix): drawing.c $(IntermediateDirectory)/drawing.c$(DependSuffix)
	$(CC) $(SourceSwitch) "drawing.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/drawing.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/drawing.c$(DependSuffix): drawing.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/drawing.c$(ObjectSuffix) -MF$(IntermediateDirectory)/drawing.c$(DependSuffix) -MM drawing.c

$(IntermediateDirectory)/drawing.c$(PreprocessSuffix): drawing.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/drawing.c$(PreprocessSuffix) drawing.c

$(IntermediateDirectory)/simplechart.c$(ObjectSuffix): simplechart.c $(IntermediateDirectory)/simplechart.c$(DependSuffix)
	$(CC) $(SourceSwitch) "simplechart.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/simplechart.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/simplechart.c$(DependSuffix): simplechart.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/simplechart.c$(ObjectSuffix) -MF$(IntermediateDirectory)/simplechart.c$(DependSuffix) -MM simplechart.c

$(IntermediateDirectory)/simplechart.c$(PreprocessSuffix): simplechart.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/simplechart.c$(PreprocessSuffix) simplechart.c

$(IntermediateDirectory)/packet_parser.c$(ObjectSuffix): packet_parser.c $(IntermediateDirectory)/packet_parser.c$(DependSuffix)
	$(CC) $(SourceSwitch) "packet_parser.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/packet_parser.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/packet_parser.c$(DependSuffix): packet_parser.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/packet_parser.c$(ObjectSuffix) -MF$(IntermediateDirectory)/packet_parser.c$(DependSuffix) -MM packet_parser.c

$(IntermediateDirectory)/packet_parser.c$(PreprocessSuffix): packet_parser.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/packet_parser.c$(PreprocessSuffix) packet_parser.c

$(IntermediateDirectory)/device_functions.c$(ObjectSuffix): device_functions.c $(IntermediateDirectory)/device_functions.c$(DependSuffix)
	$(CC) $(SourceSwitch) "device_functions.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/device_functions.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/device_functions.c$(DependSuffix): device_functions.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/device_functions.c$(ObjectSuffix) -MF$(IntermediateDirectory)/device_functions.c$(DependSuffix) -MM device_functions.c

$(IntermediateDirectory)/device_functions.c$(PreprocessSuffix): device_functions.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/device_functions.c$(PreprocessSuffix) device_functions.c

$(IntermediateDirectory)/fft.c$(ObjectSuffix): fft.c $(IntermediateDirectory)/fft.c$(DependSuffix)
	$(CC) $(SourceSwitch) "fft.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fft.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fft.c$(DependSuffix): fft.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/fft.c$(ObjectSuffix) -MF$(IntermediateDirectory)/fft.c$(DependSuffix) -MM fft.c

$(IntermediateDirectory)/fft.c$(PreprocessSuffix): fft.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fft.c$(PreprocessSuffix) fft.c

$(IntermediateDirectory)/sft.c$(ObjectSuffix): sft.c $(IntermediateDirectory)/sft.c$(DependSuffix)
	$(CC) $(SourceSwitch) "sft.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sft.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sft.c$(DependSuffix): sft.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sft.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sft.c$(DependSuffix) -MM sft.c

$(IntermediateDirectory)/sft.c$(PreprocessSuffix): sft.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sft.c$(PreprocessSuffix) sft.c

$(IntermediateDirectory)/spectrogram.c$(ObjectSuffix): spectrogram.c $(IntermediateDirectory)/spectrogram.c$(DependSuffix)
	$(CC) $(SourceSwitch) "spectrogram.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/spectrogram.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/spectrogram.c$(DependSuffix): spectrogram.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/spectrogram.c$(ObjectSuffix) -MF$(IntermediateDirectory)/spectrogram.c$(DependSuffix) -MM spectrogram.c

$(IntermediateDirectory)/spectrogram.c$(PreprocessSuffix): spectrogram.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/spectrogram.c$(PreprocessSuffix) spectrogram.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


