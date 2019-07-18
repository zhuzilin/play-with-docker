FROM ubuntu

RUN apt-get update
RUN apt-get install -qq nodejs npm
COPY ./ /var/app/
WORKDIR /var/app/

CMD npm install && npm start

EXPOSE 3000
