ARG ARCH=
FROM registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/buildenv-${ARCH}:latest

ENV USER HwHiAiUser
ENV HOME /home/$USER
ENV TOOLKIT Ascend-cann-toolkit_7.0.RC1.alpha003_linux-aarch64.run

RUN apt install -y sudo
RUN apt install -y wget

#TODO: missing gpg verification
RUN /bin/bash -c "mkdir /cann-toolkit \
  && cd /cann-toolkit \
  && wget -O ${TOOLKIT} https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/Milan-ASL/Milan-ASL%20V100R001C13SPC703/${TOOLKIT} \
  && chmod +x ./${TOOLKIT}"

RUN /bin/bash -c "cd /cann-toolkit \
    && echo 'Y' | USER=root /cann-toolkit/${TOOLKIT} --full --install-for-all \
    && cd .. \
    && rm -rf /usr/local/Ascend/ascend-toolkit/7.0.RC1.alpha003/aarch64-linux/simulator \
    && rm -rf /usr/local/Ascend/ascend-toolkit/7.0.RC1.alpha003/opp/built-in/data \
    && rm -rf cann-toolkit"

RUN useradd -d /home/$USER -m $USER -G root \
  && chown -R $USER /home/$USER  \
  && echo "$USER:Mind@123" | chpasswd \
  && chmod 750 /home/$USER \
  && echo 'umask 0027' >> ${HOME}/.bashrc \
  && adduser $USER sudo

ENV ESCAPED_LD_LIBRARY_PATH='$LD_LIBRARY_PATH'
RUN echo "export LD_LIBRARY_PATH=/usr/local/Ascend/ascend-toolkit/latest/fwkacllib/lib64:/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub:/usr/local/Ascend/driver/lib64:/usr/local/Ascend/driver/lib64/common:/usr/local/Ascend/driver/lib64/driver:$ESCAPED_LD_LIBRARY_PATH" >> /usr/local/Ascend/ascend-toolkit/set_env.sh
RUN echo "source /usr/local/Ascend/ascend-toolkit/set_env.sh" >> $HOME/.bashrc

RUN chown -R $USER /usr/local/lib
RUN chown -R $USER $HOME

WORKDIR $HOME
USER $USER 
