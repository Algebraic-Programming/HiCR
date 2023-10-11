FROM registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/buildenv:latest

ENV USER HwHiAiUser
ENV HOME /home/$USER
ENV TOOLKIT Ascend-cann-toolkit_7.0.RC1.alpha003_linux-aarch64.run

RUN apt install -y sudo
RUN apt install -y wget
RUN apt install -y python3.8-venv

#TODO: missing gpg verification
RUN mkdir /cann-toolkit \
  && cd /cann-toolkit \
  && wget -O ${TOOLKIT} https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/Milan-ASL/Milan-ASL%20V100R001C13SPC703/${TOOLKIT} 

RUN /bin/bash -c "python3 -m venv /.venv && source /.venv/bin/activate \
  && cd /cann-toolkit \ 
  && chmod +x ./${TOOLKIT} \
  && USER=root \
  && echo 'Y' | /cann-toolkit/${TOOLKIT} --full --install-for-all \
  && rm -rf /cann-toolkit rm -rf /.venv" 

RUN useradd -d /home/$USER -m $USER -G root \
  && chown -R $USER /home/$USER  \
  && echo "$USER:Mind@123" | chpasswd \
  && chmod 750 /home/$USER \
  && echo 'umask 0027' >> ${HOME}/.bashrc \
  && adduser $USER sudo

ENV ESCAPED_LD_LIBRARY_PATH='$LD_LIBRARY_PATH'
RUN echo "export LD_LIBRARY_PATH=/usr/local/Ascend/driver/lib64:/usr/local/Ascend/driver/lib64/common:/usr/local/Ascend/driver/lib64/driver:$ESCAPED_LD_LIBRARY_PATH" >> /usr/local/Ascend/ascend-toolkit/set_env.sh
RUN echo "source /usr/local/Ascend/ascend-toolkit/set_env.sh" >> $HOME/.bashrc

RUN chown -R $USER /usr/local/lib
RUN chown -R $USER $HOME

WORKDIR $HOME
USER $USER 
