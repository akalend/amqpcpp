# Include project Makefile
include Makefile

# Object Directory
objsdir=${builddir}/${CONF}

# Object Files
OBJS= \
	${objsdir}/AMQPBase.o \
	${objsdir}/AMQP.o \
	${objsdir}/AMQPException.o \
	${objsdir}/AMQPExchange.o \
	${objsdir}/AMQPMessage.o \
	${objsdir}/AMQPQueue.o

# CC Compiler Flags
CPPFLAGS+= -fPIC -MMD -MP -I${includedir}

# Link Libraries and Options
LDLIBSOPTIONS= -shared -fpic

# Build Targets
.build-conf: .pre-build ${prelibdir}/lib${PRODUCT_NAME}.so

.pre-build:
	$(MKDIR) -p ${prelibdir}
	$(MKDIR) -p ${objsdir}
	$(RM) ${objsdir}/*.d

${prelibdir}/lib${PRODUCT_NAME}.so: ${OBJS}
	${LINK.cc} -o ${prelibdir}/lib${PRODUCT_NAME}.so ${OBJS} ${LDLIBSOPTIONS} ${LIBS} 

${objsdir}/AMQPBase.o: ${srcdir}/AMQPBase.cpp 
	$(COMPILE.cc) ${CXXFLAGS} -MF $@.d -o $@ $^
${objsdir}/AMQP.o: ${srcdir}/AMQP.cpp 
	$(COMPILE.cc) ${CXXFLAGS} -MF $@.d -o $@ $^
${objsdir}/AMQPException.o: ${srcdir}/AMQPException.cpp 
	$(COMPILE.cc) ${CXXFLAGS} -MF $@.d -o $@ $^
${objsdir}/AMQPExchange.o: ${srcdir}/AMQPExchange.cpp 
	$(COMPILE.cc) ${CXXFLAGS} -MF $@.d -o $@ $^
${objsdir}/AMQPMessage.o: ${srcdir}/AMQPMessage.cpp 
	$(COMPILE.cc) ${CXXFLAGS} -MF $@.d -o $@ $^
${objsdir}/AMQPQueue.o: ${srcdir}/AMQPQueue.cpp 
	$(COMPILE.cc) ${CXXFLAGS} -MF $@.d -o $@ $^
