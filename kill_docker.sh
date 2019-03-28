docker ps | grep router | awk '{ print $1 }' | xargs docker kill
