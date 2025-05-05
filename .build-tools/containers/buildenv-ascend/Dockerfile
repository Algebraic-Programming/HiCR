FROM ghcr.io/algebraic-programming/hicr/buildenv:latest

ARG ARCH=none

ENV USER=hicr
ENV HOME=/home/$USER
ENV CANN_VERSION=7.0.RC1.alpha003

USER $USER 

RUN mkdir $HOME/cann-toolkit \
  && cd $HOME/cann-toolkit \
  &&  if [ "$ARCH" = "amd64" ]; then \
  export ARCH="x86_64"; \
  elif [ "$ARCH" = "arm64" ]; then  \
  export ARCH="aarch64"; \
  else \
  export ARCH="${ARCH}"; \
  fi \
  && export TOOLKIT=Ascend-cann-toolkit_${CANN_VERSION}_linux-$(arch).run \
  && echo "https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/Milan-ASL/Milan-ASL%20V100R001C13SPC703/$TOOLKIT" \
  && wget -q -O ${TOOLKIT} https://ascend-repo.obs.cn-east-2.myhuaweicloud.com/Milan-ASL/Milan-ASL%20V100R001C13SPC703/${TOOLKIT} \
  && chmod +x ./${TOOLKIT}

RUN cd $HOME/cann-toolkit \
  && if [ "$ARCH" = "amd64" ]; then \
  export ARCH="x86_64"; \
  elif [ "$ARCH" = "arm64" ]; then  \
  export ARCH="aarch64"; \
  else \
  export ARCH="${ARCH}"; \
  fi \
  && export TOOLKIT=Ascend-cann-toolkit_${CANN_VERSION}_linux-${ARCH}.run \
  && echo "Y" | sudo bash -E -c "source ${HOME}/.venv/bin/activate && alias pip3='python3 -m pip ' && $HOME/cann-toolkit/${TOOLKIT} --full --install-for-all" \
  && cd .. \
  && sudo rm -rf /usr/local/Ascend/ascend-toolkit/${CANN_VERSION}/${ARCH}-linux/simulator \
  && sudo rm -rf /usr/local/Ascend/ascend-toolkit/${CANN_VERSION}/opp/built-in/data \
  && rm -rf cann-toolkit

ENV ESCAPED_LD_LIBRARY_PATH='$LD_LIBRARY_PATH'
RUN echo "LD_LIBRARY_PATH=/usr/local/Ascend/driver/lib64:/usr/local/Ascend/driver/lib64/common:/usr/local/Ascend/driver/lib64/driver:$ESCAPED_LD_LIBRARY_PATH" >> $HOME/.hicr-env.sh
RUN echo "source /usr/local/Ascend/ascend-toolkit/set_env.sh" >> $HOME/.hicr-env.sh

WORKDIR $HOME
