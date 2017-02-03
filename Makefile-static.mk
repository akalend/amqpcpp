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

# C Compiler Flags
CFLAGS+= -MMD -MP

# CC Compiler Flags
CPPFLAGS+= -I${includedir} -MMD -MP

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: .pre-build ${prelibdir}/lib${PRODUCT_NAME}.a

.pre-build:
	$(MKDIR) -p ${prelibdir}
	$(MKDIR) -p ${objsdir}
	$(RM) ${objsdir}/*.d
	$(RM) ${prelibdir}/lib${PRODUCT_NAME}.a

${prelibdir}/lib${PRODUCT_NAME}.a: ${OBJS}
	${AR} rv ${prelibdir}/lib${PRODUCT_NAME}.a ${OBJS} 
	$(RANLIB) ${prelibdir}/lib${PRODUCT_NAME}.a

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
