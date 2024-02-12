ARG ARCH=
FROM registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/buildenv-${ARCH}:latest

ENV USER hicr
ENV HOME /home/$USER
ENV CANN_VERSION 7.0.RC1.alpha003
ENV TOOLKIT Ascend-cann-toolkit_${CANN_VERSION}_linux-aarch64.run

#TODO: missing gpg verification
RUN /bin/bash -c "mkdir $HOME/cann-toolkit \
  && cd $HOME/cann-toolkit \
  && wget -O ${TOOLKIT} https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/Milan-ASL/Milan-ASL%20V100R001C13SPC703/${TOOLKIT} \
  && chmod +x ./${TOOLKIT}"

RUN /bin/bash -c "cd $HOME/cann-toolkit \
    && echo 'Y' | sudo $HOME/cann-toolkit/${TOOLKIT} --full --install-for-all \
    && cd .. \
    && sudo rm -rf /usr/local/Ascend/ascend-toolkit/${CANN_VERSION}/aarch64-linux/simulator \
    && sudo rm -rf /usr/local/Ascend/ascend-toolkit/${CANN_VERSION}/opp/built-in/data \
    && rm -rf cann-toolkit"

ENV ESCAPED_LD_LIBRARY_PATH='$LD_LIBRARY_PATH'
RUN echo "LD_LIBRARY_PATH=/usr/local/Ascend/driver/lib64:/usr/local/Ascend/driver/lib64/common:/usr/local/Ascend/driver/lib64/driver:$ESCAPED_LD_LIBRARY_PATH" >> $HOME/.hicr-env.sh
RUN echo "source /usr/local/Ascend/ascend-toolkit/set_env.sh" >> $HOME/.hicr-env.sh