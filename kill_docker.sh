docker ps | grep router | awk '{ print  }' | xargs docker kill
